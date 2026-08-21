#ifndef SLOPOS_MOUSE_H
#define SLOPOS_MOUSE_H

#include "types.h"
#include "idt.h"

#define MOUSE_BTN_LEFT   1
#define MOUSE_BTN_RIGHT  2
#define MOUSE_BTN_MIDDLE 4

typedef struct {
    u32 x, y;            /* absolute position */
    i8  dx, dy;          /* last delta */
    u8  buttons;         /* current button state */
    i8  wheel;           /* scroll delta */
} mouse_state_t;

typedef void (*mouse_handler_t)(mouse_state_t *st);

void mouse_init(void);
void mouse_set_handler(mouse_handler_t h);
void mouse_get_state(mouse_state_t *st);

#endif
