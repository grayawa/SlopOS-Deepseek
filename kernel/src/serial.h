/* SlopOS Serial Port Driver
 * SPDX-License-Identifier: 0BSD
 */
#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include "io.h"

#define COM1 0x3F8

static inline int serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
    outb(COM1 + 4, 0x1E);
    outb(COM1 + 0, 0xAE);
    if (inb(COM1 + 0) != 0xAE) return 0;
    outb(COM1 + 4, 0x0F);
    return 1;
}

static inline int serial_received(void) {
    return inb(COM1 + 5) & 1;
}

static inline char serial_read(void) {
    while (!serial_received()) { __asm__ volatile("pause"); }
    return inb(COM1 + 0);
}

static inline int serial_is_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

static inline void serial_write(char c) {
    while (!serial_is_transmit_empty()) { __asm__ volatile("pause"); }
    outb(COM1 + 0, c);
}

static inline void serial_write_str(const char *s) {
    while (*s) {
        if (*s == '\n') serial_write('\r');
        serial_write(*s++);
    }
}

#endif
