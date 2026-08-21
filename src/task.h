#ifndef SLOPOS_TASK_H
#define SLOPOS_TASK_H

#include "types.h"

#define TASK_READY 0
#define TASK_RUNNING 1
#define TASK_DEAD 2
#define TASK_BLOCKED 3

typedef struct task {
    /* context (must match task.S offsets) */
    u64 rsp, rbp, rbx, r12, r13, r14, r15;   /* 0..48 */
    u64 pml4;                                 /* 56 */
    /* end of asm-relevant fields */
    u64 entry;                                /* user entry / kernel fn */
    u64 kstack_top;
    u64 kstack_bottom;
    u64 user_rsp;
    u64 user_rip;
    int  is_user;
    int  state;
    u64 exit_code;
    int  id;
    char name[32];
    struct task *next;
} task_t;

void sched_init(void);
task_t *task_create_kernel(const char *name, void (*fn)(void));
task_t *task_create_user(const char *name, u64 entry, u64 user_stack_top, u64 pml4);
void sched_yield(void);
void sched_exit(u64 code);
void sched_tick(void);
void sched_run(void);          /* start the scheduler (never returns) */
task_t *sched_current(void);
void sched_lock(void);
void sched_unlock(void);

#endif
