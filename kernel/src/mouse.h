/* SlopOS PS/2 Mouse Driver
 * SPDX-License-Identifier: 0BSD
 */
#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_init(void);
void mouse_handle(uint8_t data);
int mouse_get_x(void);
int mouse_get_y(void);
int mouse_get_buttons(void);
int mouse_get_scroll(void);

#endif
