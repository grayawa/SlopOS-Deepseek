#ifndef SLOPOS_WM_H
#define SLOPOS_WM_H

#include "types.h"
#include "keyboard.h"
#include "mouse.h"

#define TITLE_BAR_H   22
#define WINDOW_BORDER 1
#define WM_MAX_WINDOWS 16

struct window;

typedef struct window window_t;

/* per-window callbacks */
typedef void (*win_draw_fn)(window_t *w);
typedef void (*win_key_fn)(window_t *w, struct key_event ev);
typedef void (*win_mouse_fn)(window_t *w, mouse_state_t *ms);
typedef void (*win_close_fn)(window_t *w);

struct window {
    int id;
    int x, y, w, h;
    int visible;
    int focused;
    char title[64];

    win_draw_fn  draw;
    win_key_fn   key;
    win_mouse_fn mouse;
    win_close_fn close;

    void *user;            /* per-window private data */

    /* client-area geometry helpers */
    int cx, cy, cw, ch;

    struct window *next;
};

void wm_init(void);
window_t *wm_create_window(int x, int y, int w, int h, const char *title,
                           win_draw_fn draw, win_key_fn key, win_mouse_fn mouse,
                           win_close_fn close);
void wm_destroy_window(window_t *w);
void wm_focus(window_t *w);
window_t *wm_focused(void);
void wm_invalidate(window_t *w);     /* mark dirty, schedules a redraw */
void wm_redraw(void);                /* full repaint of desktop + windows + cursor */
int  wm_is_dirty(void);              /* non-zero if a repaint is pending */
void wm_clear_dirty(void);
void wm_handle_key(struct key_event ev);
void wm_handle_mouse(mouse_state_t *ms);
void wm_raise(window_t *w);          /* bring to front */

/* background / drawing helpers used by windows */
void wm_draw_titlebar(window_t *w);
void wm_set_bg(u32 color);
void wm_set_accent(u32 color);
void wm_draw_window_frame(window_t *w);

#endif
