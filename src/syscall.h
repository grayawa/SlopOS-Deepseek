#ifndef SLOPOS_SYSCALL_H
#define SLOPOS_SYSCALL_H

#include "types.h"

struct syscall_frame {
    u64 rax;                 /* syscall number */
    u64 rdi, rsi, rdx, r10, r8, r9;   /* args 0..5 */
    u64 user_rip, user_rflags, user_rsp;
};

void syscall_init(void);
u64 syscall_dispatch(struct syscall_frame *f);

/* console output routing (used by write() to stdout/stderr) */
void console_set_writer(void (*fn)(const char *buf, size_t len));
void console_write(const char *buf, size_t len);

#endif
