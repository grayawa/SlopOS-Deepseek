#include "serial.h"

#define COM1 0x3F8

static int serial_initialized = 0;

static inline void outb(u16 port, u8 val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port)
{
    u8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void)
{
    outb(COM1 + 1, 0x00);    /* disable interrupts */
    outb(COM1 + 3, 0x80);    /* enable DLAB */
    outb(COM1 + 0, 0x03);    /* divisor low (38400 baud) */
    outb(COM1 + 1, 0x00);    /* divisor high */
    outb(COM1 + 3, 0x03);    /* 8N1 */
    outb(COM1 + 2, 0xC7);    /* FIFO enable, clear */
    outb(COM1 + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
    serial_initialized = 1;
}

static int serial_tx_empty(void)
{
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(char c)
{
    if (!serial_initialized)
        return;
    if (c == '\n')
        serial_putc('\r');
    while (!serial_tx_empty())
        ;
    outb(COM1, (u8)c);
}

void serial_write(const char *s)
{
    while (*s)
        serial_putc(*s++);
}

void serial_hex(u64 value)
{
    static const char *digits = "0123456789abcdef";
    int i;
    serial_write("0x");
    for (i = 15; i >= 0; i--)
        serial_putc(digits[(value >> (i * 4)) & 0xF]);
}

void serial_dec(u64 value)
{
    char buf[24];
    int i = 0;
    if (value == 0) {
        serial_putc('0');
        return;
    }
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    while (i > 0)
        serial_putc(buf[--i]);
}
