/* SlopOS PS/2 Mouse Driver
 * SPDX-License-Identifier: 0BSD
 */
#include "mouse.h"
#include "io.h"
#include "serial.h"

static int mouse_x = 400, mouse_y = 300;
static int mouse_buttons = 0;
static int mouse_scroll = 0;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_bytes[4];

static inline void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

static inline void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(0);
    outb(0x60, data);
}

static inline uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init(void) {
    uint8_t status;

    /* Enable auxiliary device */
    mouse_wait(1);
    outb(0x64, 0xA8);

    /* Enable interrupts */
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = inb(0x60) | 2;
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    /* Set defaults */
    mouse_write(0xF6);
    mouse_read();

    /* Enable */
    mouse_write(0xF4);
    mouse_read();

    serial_write_str("[Mouse] PS/2 mouse initialized\n");
}

void mouse_handle(uint8_t data) {
    switch (mouse_cycle) {
    case 0:
        if (!(data & 0x08)) break; /* Not aligned */
        mouse_bytes[0] = data;
        mouse_cycle++;
        break;
    case 1:
        mouse_bytes[1] = data;
        mouse_cycle++;
        break;
    case 2:
        mouse_bytes[2] = data;
        mouse_cycle = 0;

        mouse_buttons = mouse_bytes[0] & 0x07;

        int dx = mouse_bytes[1];
        int dy = mouse_bytes[2];
        if (mouse_bytes[0] & 0x10) dx |= ~0xFF;
        if (mouse_bytes[0] & 0x20) dy |= ~0xFF;

        mouse_x += dx;
        mouse_y -= dy;

        if (mouse_x < 0) mouse_x = 0;
        if (mouse_y < 0) mouse_y = 0;
        /* Clamp to screen will be done by caller */

        mouse_scroll = 0;
        if (mouse_bytes[3]) {
            mouse_scroll = (int8_t)mouse_bytes[3];
        }
        break;
    }
}

int mouse_get_x(void) { return mouse_x; }
int mouse_get_y(void) { return mouse_y; }
int mouse_get_buttons(void) { return mouse_buttons; }
int mouse_get_scroll(void) { return mouse_scroll; }

void mouse_set_bounds(int max_x, int max_y) {
    if (mouse_x >= max_x) mouse_x = max_x - 1;
    if (mouse_y >= max_y) mouse_y = max_y - 1;
}
