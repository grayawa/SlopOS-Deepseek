#ifndef SLOPOS_KEYBOARD_H
#define SLOPOS_KEYBOARD_H

#include "types.h"
#include "idt.h"

#define KEY_MOD_SHIFT 1
#define KEY_MOD_CTRL  2
#define KEY_MOD_ALT   4
#define KEY_MOD_CAPS  8

struct key_event {
    u8 scancode;
    u8 press;       /* 1 = key down, 0 = key up */
    u8 ascii;       /* translated character (0 if none) */
    u8 modifiers;
};

typedef void (*key_handler_t)(struct key_event ev);

void kbd_init(void);
void kbd_set_handler(key_handler_t h);
int  kbd_get_event(struct key_event *ev);   /* dequeue, returns 1 if available */

#endif
