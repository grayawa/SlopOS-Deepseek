/* SlopOS Process Scheduler Implementation
 * SPDX-License-Identifier: 0BSD
 */
#include "proc.h"
#include "pmm.h"
#include "gdt.h"
#include "serial.h"

static struct process *process_list = NULL;
static struct process *current_process = NULL;
static int next_pid = 1;
static int scheduler_enabled = 0;

void proc_init(void) {
    process_list = NULL;
    current_process = NULL;
    serial_write_str("[PROC] Process subsystem initialized\n");
}

struct process *proc_create(void (*entry)(void), uint64_t flags) {
    (void)flags;
    struct process *proc = (struct process *)pmm_alloc_page();
    if (!proc) return NULL;

    proc->pid = next_pid++;
    proc->state = PROC_READY;
    proc->exit_code = 0;

    /* Set up kernel stack for initial context */
    uint64_t *kstack = proc->kstack + KSTACK_SIZE / 8;
    struct context *ctx = (struct context *)(kstack - sizeof(struct context)/8 - 1);
    proc->stack_top = ctx;

    /* Initialize context for first switch */
    for (int i = 0; i < (int)(sizeof(struct context)/8); i++) ((uint64_t*)ctx)[i] = 0;
    ctx->rip = (uint64_t)entry;
    ctx->cs = 0x08;
    ctx->rflags = 0x202;
    ctx->rsp = (uint64_t)kstack;
    ctx->ss = 0x10;
    ctx->rbp = (uint64_t)kstack;

    proc->ctx = *ctx;

    /* Add to process list */
    proc->next = process_list;
    process_list = proc;

    return proc;
}

struct process *proc_current(void) {
    return current_process;
}

int proc_getpid(void) {
    return current_process ? current_process->pid : 0;
}

void proc_exit(int code) {
    if (current_process) {
        current_process->exit_code = code;
        current_process->state = PROC_ZOMBIE;
    }
    proc_yield();
}

/* Switch to the next process. Called from assembly. */
struct process *schedule_next(void) {
    if (!current_process) {
        current_process = process_list;
        return current_process;
    }

    /* Find current in list, move to next */
    struct process *next = current_process->next;
    if (!next) next = process_list;
    current_process = next;
    return current_process;
}

void schedule(void) {
    if (!scheduler_enabled) return;
    proc_yield();
}

void proc_yield(void) {
    /* Will be called from timer ISR */
    if (!current_process || !current_process->next) {
        if (!current_process) current_process = process_list;
        return;
    }
    /* Switch context to next process */
    struct process *next = schedule_next();
    if (next == current_process) return;

    struct process *prev = current_process;
    current_process = next;

    /* Do context switch */
    extern void context_switch(struct context *old, struct context *new);
    context_switch(&prev->ctx, &next->ctx);
}

void scheduler_start(void) {
    scheduler_enabled = 1;
    current_process = process_list;
    if (!current_process) {
        serial_write_str("[PROC] No processes to schedule!\n");
        return;
    }
    serial_write_str("[PROC] Starting scheduler...\n");

    /* Load the first process context */
    tss_set_stack((uint64_t)current_process->stack_top);
    __asm__ volatile(
        "mov %0, %%rsp\n"
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%r11\n"
        "pop %%r10\n"
        "pop %%r9\n"
        "pop %%r8\n"
        "pop %%rbp\n"
        "pop %%rdi\n"
        "pop %%rsi\n"
        "pop %%rdx\n"
        "pop %%rcx\n"
        "pop %%rbx\n"
        "pop %%rax\n"
        "iretq\n"
        :
        : "r"(&current_process->ctx.r15)
        : "memory"
    );
}
