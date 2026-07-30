/* SlopOS Process/Task Management
 * SPDX-License-Identifier: 0BSD
 */
#ifndef PROC_H
#define PROC_H

#include <stdint.h>

#define MAX_PROCESSES 64
#define KSTACK_SIZE   8192

enum proc_state {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_ZOMBIE
};

struct context {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

struct process {
    int pid;
    enum proc_state state;
    struct context ctx;
    uint64_t *page_table;
    void *stack_top;
    uint64_t kstack[KSTACK_SIZE / 8];
    int exit_code;
    struct process *next;
};

void proc_init(void);
struct process *proc_create(void (*entry)(void), uint64_t flags);
void proc_yield(void);
void proc_exit(int code);
void schedule(void);
struct process *proc_current(void);
int proc_getpid(void);

/* Start the scheduler - never returns */
void scheduler_start(void);

#endif
