/* SlopOS Window Manager
 * SPDX-License-Identifier: 0BSD
 */
#ifndef WM_H
#define WM_H

#include <stdint.h>

#define MAX_WINDOWS 32
#define WIN_TITLE_H 20

struct window {
    int id;
    int x, y, w, h;
    char title[64];
    uint32_t *buffer;     /* Client area pixel buffer */
    int visible;
    int focused;
    int dragging;
    int drag_off_x, drag_off_y;
};

/* Window content callbacks */
typedef void (*win_draw_fn)(struct window *win);

void wm_init(int screen_w, int screen_h);
struct window *wm_create_window(int x, int y, int w, int h, const char *title);
void wm_destroy_window(struct window *win);
void wm_draw_all(void);
void wm_handle_mouse(int mx, int my, int buttons);
struct window *wm_window_at(int mx, int my);
void wm_focus_window(struct window *win);
void wm_redraw_window(struct window *win);

/* Framebuffer access from outside */
extern uint32_t *get_fb(void);
extern int get_screen_w(void);
extern int get_screen_h(void);

#endif
