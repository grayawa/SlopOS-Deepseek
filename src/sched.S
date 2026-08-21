/* Task context switch and user-mode entry for SlopOS. */
.section .text
.code64

/* void context_switch(task_t *prev, task_t *next)
 * prev/next are task_t*; the context fields are at offsets 0..56.
 */
.global context_switch
.type context_switch, @function
context_switch:
    movq %rsp, 0(%rdi)
    movq %rbp, 8(%rdi)
    movq %rbx, 16(%rdi)
    movq %r12, 24(%rdi)
    movq %r13, 32(%rdi)
    movq %r14, 40(%rdi)
    movq %r15, 48(%rdi)

    movq 0(%rsi), %rsp
    movq 8(%rsi), %rbp
    movq 16(%rsi), %rbx
    movq 24(%rsi), %r12
    movq 32(%rsi), %r13
    movq 40(%rsi), %r14
    movq 48(%rsi), %r15
    movq 56(%rsi), %rax
    movq %rax, %cr3
    ret

/* Enter user mode. The stack must be laid out (low to high) as:
 *   return address (already popped by context_switch),
 *   user_rip, user_cs, rflags, user_rsp, user_ss
 * task_enter_user just iretq's to that frame.
 */
.global task_enter_user
.type task_enter_user, @function
task_enter_user:
    iretq

.section .note.GNU-stack,"",@progbits
