#include "task.h"
#include "pmm.h"
#include "vmm.h"
#include "kmalloc.h"
#include "lib.h"
#include "printk.h"
#include "gdt.h"
#include "port.h"

/* context_switch and task_enter_user are defined in task.S */
extern void context_switch(task_t *prev, task_t *next);
extern void task_enter_user(void);

static task_t *current;
static task_t *ready_head;   /* circular list of runnable tasks */
static int next_id = 1;
static volatile int sched_locked;
static volatile u64 current_ticks;
#define TIME_SLICE 5          /* ticks per time slice */

/* boot/main-loop task: represents the kernel's initial execution context */
static task_t boot_task;

void sched_lock(void)   { sched_locked++; }
void sched_unlock(void) { if (sched_locked > 0) sched_locked--; }

task_t *sched_current(void)
{
    return current;
}

static void list_add(task_t *t)
{
    if (!ready_head) {
        ready_head = t;
        t->next = t;
    } else {
        t->next = ready_head->next;
        ready_head->next = t;
    }
}

void sched_init(void)
{
    memset((void *)&boot_task, 0, sizeof(boot_task));
    boot_task.id = next_id++;
    boot_task.pml4 = vmm_kernel_pml4();
    boot_task.state = TASK_RUNNING;
    boot_task.is_user = 0;
    strcpy(boot_task.name, "boot");
    current = &boot_task;
    ready_head = &boot_task;
    boot_task.next = &boot_task;
    sched_locked = 0;
}

task_t *task_create_kernel(const char *name, void (*fn)(void))
{
    task_t *t = (task_t *)malloc(sizeof(task_t));
    if (!t) return NULL;
    memset((void *)t, 0, sizeof(task_t));

    u64 kstack = pmm_alloc_contiguous(4);   /* 16 KiB */
    if (!kstack) { free(t); return NULL; }
    t->kstack_bottom = kstack;
    t->kstack_top = kstack + 16384;
    t->pml4 = vmm_kernel_pml4();
    t->is_user = 0;

    /* stack: fn is the return target for the initial context_switch */
    *(u64 *)(t->kstack_top - 8) = (u64)fn;
    t->rsp = t->kstack_top - 8;

    t->id = next_id++;
    t->state = TASK_READY;
    strncpy(t->name, name, 31);
    list_add(t);
    kprintf("[task] kernel task %s id=%d\n", name, t->id);
    return t;
}

task_t *task_create_user(const char *name, u64 entry, u64 user_stack_top, u64 pml4)
{
    task_t *t = (task_t *)malloc(sizeof(task_t));
    if (!t) return NULL;
    memset((void *)t, 0, sizeof(task_t));

    u64 kstack = pmm_alloc_contiguous(4);
    if (!kstack) { free(t); return NULL; }
    t->kstack_bottom = kstack;
    t->kstack_top = kstack + 16384;
    t->pml4 = pml4;
    t->is_user = 1;
    t->user_rip = entry;
    t->user_rsp = user_stack_top;

    /* Build the iretq frame on the kernel stack (low to high):
     *   task_enter_user, user_rip, user_cs, rflags, user_rsp, user_ss */
    u64 *sp = (u64 *)t->kstack_top;
    *--sp = 0x23;                 /* user ss  */
    *--sp = user_stack_top;       /* user rsp */
    *--sp = 0x2;                  /* rflags (IF clear: no preemption of user) */
    *--sp = 0x1B;                 /* user cs  */
    *--sp = entry;                /* user rip */
    *--sp = (u64)task_enter_user; /* ret target for context_switch */
    t->rsp = (u64)sp;

    t->id = next_id++;
    t->state = TASK_READY;
    strncpy(t->name, name, 31);
    list_add(t);
    kprintf("[task] user task %s id=%d entry=0x%llx\n", name, t->id, entry);
    return t;
}

/* pick the next runnable task, round-robin */
static task_t *pick_next(task_t *from)
{
    task_t *t = from;
    for (;;) {
        t = t->next;
        if (t == from) return from;   /* only one runnable task (or none) */
        if (t->state == TASK_READY || t->state == TASK_RUNNING)
            return t;
        if (t->next == from) return from;
    }
}

void sched_yield(void)
{
    task_t *prev = current;
    task_t *next = pick_next(prev);
    if (next == prev)
        return;
    prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    current = next;
    /* update the kernel stack used on syscall/interrupt for user tasks */
    if (next->is_user) {
        extern u64 syscall_kstack;
        syscall_kstack = next->kstack_top;
        gdt_set_tss_rsp0(next->kstack_top);
    }
    current_ticks = 0;
    context_switch(prev, next);
}

void sched_exit(u64 code)
{
    task_t *prev = current;
    prev->state = TASK_DEAD;
    prev->exit_code = code;
    kprintf("[task] %s exited (%llu)\n", prev->name, code);

    task_t *next = pick_next(prev);
    if (next == prev || next->state == TASK_DEAD) {
        /* nothing else to run; halt */
        kputs("[task] no more tasks; halting\n");
        cli();
        for (;;) hlt();
    }
    next->state = TASK_RUNNING;
    current = next;
    if (next->is_user) {
        extern u64 syscall_kstack;
        syscall_kstack = next->kstack_top;
        gdt_set_tss_rsp0(next->kstack_top);
    }
    current_ticks = 0;
    context_switch(prev, next);
}

void sched_tick(void)
{
    if (sched_locked)
        return;
    if (current && current->state == TASK_RUNNING) {
        current_ticks++;
        if (current_ticks >= TIME_SLICE)
            sched_yield();
    }
}

void sched_run(void)
{
    task_t *prev = current;
    task_t *next = pick_next(prev);
    if (next == prev) return;
    next->state = TASK_RUNNING;
    current = next;
    if (next->is_user) {
        extern u64 syscall_kstack;
        syscall_kstack = next->kstack_top;
        gdt_set_tss_rsp0(next->kstack_top);
    }
    current_ticks = 0;
    context_switch(prev, next);
}
