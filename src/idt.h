#ifndef SLOPOS_IDT_H
#define SLOPOS_IDT_H

#include "types.h"

typedef struct {
    u64 r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 int_no;
    u64 err_code;
    u64 rip, cs, rflags, rsp, ss;   /* rsp/ss only meaningful for ring-3 source */
} isr_frame_t;

/* exception / IRQ names */
extern const char *exception_names[32];

void idt_init(void);
void isr_handler(isr_frame_t *frame);

/* register a handler for a PIC IRQ (0..15). Returns previous. */
typedef void (*irq_handler_t)(isr_frame_t *frame);
irq_handler_t irq_register_handler(int irq, irq_handler_t handler);

#endif
