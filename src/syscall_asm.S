/* Linux-compatible syscall entry (via the `syscall` instruction). */
.section .text
.code64

/* The scheduler stores the current user task's kernel stack here before
 * entering user mode. On syscall, RSP is the user stack; we swap to this. */
.global syscall_kstack
syscall_kstack:
    .quad 0

/* syscall_frame layout (offsets from the pointer passed to syscall_dispatch):
 *   0 rax (syscall number)
 *   8 rdi
 *  16 rsi
 *  24 rdx
 *  32 r10
 *  40 r8
 *  48 r9
 *  56 user_rip
 *  64 user_rflags
 *  72 user_rsp
 */
.global syscall_entry
.type syscall_entry, @function
syscall_entry:
    /* rcx = user rip, r11 = user rflags (set by `syscall`) */
    /* swap to kernel stack; keep user rsp in rbx */
    movq syscall_kstack(%rip), %rbx
    xchgq %rsp, %rbx

    /* push the frame (reverse order of the struct) */
    pushq %rbx               /* 72 user_rsp */
    pushq %r11               /* 64 user_rflags */
    pushq %rcx               /* 56 user_rip  */
    pushq %r9                /* 48 arg5 */
    pushq %r8                /* 40 arg4 */
    pushq %r10               /* 32 arg3 */
    pushq %rdx               /* 24 arg2 */
    pushq %rsi               /* 16 arg1 */
    pushq %rdi               /*  8 arg0 */
    pushq %rax               /*  0 syscall number */

    movq %rsp, %rdi          /* frame pointer */
    call syscall_dispatch

    /* rax = syscall result; restore user context for sysret */
    movq 56(%rsp), %rcx      /* user_rip -> rcx */
    movq 64(%rsp), %r11      /* user_rflags -> r11 */
    movq 72(%rsp), %rsp      /* user_rsp -> rsp */
    sysretq

.section .note.GNU-stack,"",@progbits
