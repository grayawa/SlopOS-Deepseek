#include "idt.h"
#include "port.h"
#include "printk.h"
#include "isr_table.h"

#define IDT_ENTRIES 256

struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8  ist;
    u8  attr;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];

static void idt_set_gate(int idx, void (*handler)(void), u8 attr, u8 ist)
{
    u64 off = (u64)handler;
    idt[idx].offset_low  = (u16)(off & 0xFFFF);
    idt[idx].selector    = 0x08;         /* kernel code */
    idt[idx].ist         = ist;
    idt[idx].attr        = attr;
    idt[idx].offset_mid  = (u16)((off >> 16) & 0xFFFF);
    idt[idx].offset_high = (u32)(off >> 32);
    idt[idx].zero        = 0;
}

/* ---- PIC remap (8259) ---- */
static void pic_remap(void)
{
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();   /* master offset 0x20 */
    outb(0xA1, 0x28); io_wait();   /* slave offset 0x28 */
    outb(0x21, 0x04); io_wait();   /* master has slave on IRQ2 */
    outb(0xA1, 0x02); io_wait();   /* slave cascade */
    outb(0x21, 0x01); io_wait();   /* 8086 mode */
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0xFF & ~0x1F);      /* unmask IRQ0-4 */
    outb(0xA1, 0xFF & ~0x10);      /* unmask IRQ12 (mouse) */
}

static void pic_eoi(u8 irq)
{
    if (irq >= 8)
        outb(0xA0, 0x20);   /* slave EOI */
    outb(0x20, 0x20);       /* master EOI */
}

void idt_init(void)
{
    int i;
    for (i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, isr_table[i], 0x8E, 0);   /* present, DPL0, interrupt gate */

    /* Allow DPL3 int 0x80 (legacy syscall trap) - type 0xEE */
    idt_set_gate(0x80, isr_table[0x80], 0xEE, 0);

    struct {
        u16 limit;
        u64 base;
    } __attribute__((packed)) idtr = { sizeof(idt) - 1, (u64)idt };

    __asm__ volatile ("lidt %0" : : "m"(idtr));

    pic_remap();
}

static irq_handler_t irq_handlers[16];

irq_handler_t irq_register_handler(int irq, irq_handler_t handler)
{
    irq_handler_t prev = irq_handlers[irq];
    irq_handlers[irq] = handler;
    return prev;
}

const char *exception_names[32] = {
    "Divide-by-zero", "Debug", "NMI", "Breakpoint", "Overflow",
    "Bound range", "Invalid opcode", "Device not available", "Double fault",
    "Coprocessor segment overrun", "Invalid TSS", "Segment not present",
    "Stack-segment fault", "General protection", "Page fault",
    "Reserved", "x87 FP error", "Alignment check", "Machine check", "SIMD FP",
    "Virtualization", "Control protection", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "VMM comm", "Security", "Reserved"
};

static const char *exc_name(int n)
{
    if (n >= 0 && n < 32)
        return exception_names[n];
    return "Unknown";
}

void isr_handler(isr_frame_t *frame)
{
    int n = (int)frame->int_no;

    if (n < 32) {
        /* CPU exception */
        kprintf("\nEXCEPTION %d (%s) at RIP=0x%llx CS=0x%llx RFLAGS=0x%llx ERR=0x%llx CR2=0x%llx\n",
                n, exc_name(n), frame->rip, frame->cs, frame->rflags,
                frame->err_code, read_cr2());
        kprintf("  rax=0x%llx rbx=0x%llx rcx=0x%llx rdx=0x%llx\n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
        kprintf("  rdi=0x%llx rsi=0x%llx rbp=0x%llx rsp=0x%llx\n", frame->rdi, frame->rsi, frame->rbp, frame->rsp);
        kprintf("  r8 =0x%llx r9 =0x%llx r10=0x%llx r11=0x%llx\n", frame->r8, frame->r9, frame->r10, frame->r11);
        kprintf("  r12=0x%llx r13=0x%llx r14=0x%llx r15=0x%llx\n", frame->r12, frame->r13, frame->r14, frame->r15);
        kputs("System halted.\n");
        cli();
        for (;;) hlt();
    } else if (n >= 32 && n < 48) {
        int irq = n - 32;
        if (irq_handlers[irq])
            irq_handlers[irq](frame);
        pic_eoi((u8)irq);
    } else if (n == 0x80) {
        /* legacy int 0x80 syscall */
        if (irq_handlers[0])
            irq_handlers[0](frame);
    } else {
        kprintf("Unhandled interrupt %d\n", n);
        pic_eoi((u8)(n - 32));
    }
}
