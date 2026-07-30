/* SlopOS IDT and Interrupt Handlers
 * SPDX-License-Identifier: 0BSD
 */
#include "idt.h"
#include "io.h"
#include "serial.h"

#define PIC1        0x20
#define PIC2        0xA0
#define PIC1_CMD    PIC1
#define PIC1_DATA   (PIC1 + 1)
#define PIC2_CMD    PIC2
#define PIC2_DATA   (PIC2 + 1)
#define PIC_EOI     0x20

#define ICW1_ICW4   0x01
#define ICW1_INIT   0x10

static struct idt_entry idt[256];
static struct idt_ptr idt_ptr;

typedef void (*isr_t)(void);

extern isr_t isr_table[];

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_mid = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].flags = flags;
    idt[num].reserved = 0;
}

static void pic_remap(void) {
    uint8_t m1 = inb(PIC1_DATA);
    uint8_t m2 = inb(PIC2_DATA);

    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();

    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    outb(PIC1_DATA, m1);
    outb(PIC2_DATA, m2);
}

void interrupts_enable(void) {
    __asm__ volatile("sti");
}

void interrupts_disable(void) {
    __asm__ volatile("cli");
}

/* Exception handler names */
static const char *exception_names[] = {
    "Division Error", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bound Range", "Invalid Opcode", "Device Not Available",
    "Double Fault", "Coprocessor Segment", "Invalid TSS", "Segment Not Present",
    "Stack Fault", "General Protection", "Page Fault", "Reserved",
    "x87 FPU", "Alignment Check", "Machine Check", "SIMD FP",
    "Virtualization", "Control Protection", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor", "VMM Communication", "Security", "Reserved"
};

void exception_handler(uint64_t vector, uint64_t error_code, uint64_t rip) {
    serial_write_str("[SlopOS] EXCEPTION: ");
    if (vector < 32) {
        serial_write_str(exception_names[vector]);
    }
    serial_write_str(" (vector ");
    char buf[4];
    buf[0] = "0123456789ABCDEF"[(vector >> 4) & 0xF];
    buf[1] = "0123456789ABCDEF"[vector & 0xF];
    buf[2] = ')'; buf[3] = 0;
    serial_write_str(buf);
    serial_write_str(" error=");
    for (int i = 15; i >= 0; i--) {
        char c = "0123456789ABCDEF"[(error_code >> (i*4)) & 0xF];
        serial_write(c);
    }
    serial_write_str(" rip=");
    for (int i = 15; i >= 0; i--) {
        char c = "0123456789ABCDEF"[(rip >> (i*4)) & 0xF];
        serial_write(c);
    }
    serial_write_str("\n");
    for (;;) { __asm__("hlt"); }
}

/* Scancode set 1 -> ASCII (US keyboard) */
static const char scancode_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0
};

static const char scancode_shifted[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0
};

static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;

#define KEYBUF_SIZE 256
static char keybuf[KEYBUF_SIZE];
static volatile int keybuf_head = 0;
static volatile int keybuf_tail = 0;

int keyboard_getchar(void) {
    if (keybuf_head == keybuf_tail) return -1;
    char c = keybuf[keybuf_tail];
    keybuf_tail = (keybuf_tail + 1) % KEYBUF_SIZE;
    return c;
}

static void keyboard_handle(uint8_t scancode) {
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
        return;
    }
    if (scancode == 0x1D) {
        ctrl_pressed = 1;
        return;
    }
    if (scancode == 0x9D) {
        ctrl_pressed = 0;
        return;
    }
    if (scancode == 0x38) {
        alt_pressed = 1;
        return;
    }
    if (scancode == 0xB8) {
        alt_pressed = 0;
        return;
    }

    if (scancode & 0x80) return;

    if (scancode < sizeof(scancode_ascii)) {
        char c = shift_pressed ? scancode_shifted[scancode] : scancode_ascii[scancode];
        if (c) {
            int next = (keybuf_head + 1) % KEYBUF_SIZE;
            if (next != keybuf_tail) {
                keybuf[keybuf_head] = c;
                keybuf_head = next;
            }
        }
    }
}

void irq_handler(uint64_t irq, uint64_t rip) {
    if (irq == 1) {
        uint8_t scancode = inb(0x60);
        keyboard_handle(scancode);
    }
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

void idt_init(void) {
    /* Load the ISR table addresses */
    extern uint64_t isr_stub_table[];
    for (int i = 0; i < 48; i++) {
        idt_set_gate(i, isr_stub_table[i], 0x08, 0x8E);
    }

    /* Set up necessary IST or special handlers */
    idt_set_gate(8, isr_stub_table[32], 0x08, 0x8E); /* Double fault with error code */

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;

    __asm__ volatile("lidt %0" : : "m"(idt_ptr));

    pic_remap();

    /* Enable keyboard IRQ (IRQ1) */
    outb(PIC1_DATA, inb(PIC1_DATA) & ~0x02);

    serial_write_str("[SlopOS] IDT initialized, PIC remapped, interrupts enabled\n");
    interrupts_enable();
}
