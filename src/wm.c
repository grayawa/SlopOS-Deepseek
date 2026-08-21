#include "wm.h"
#include "fb.h"
#include "lib.h"
#include "kmalloc.h"
#include "timer.h"
#include "printk.h"

/* desktop colors */
static u32 c_desktop, c_taskbar, c_tasktext, c_winbg, c_title, c_title_inactive,
           c_border, c_titletext, c_close, c_close_hover;

static window_t *head;         /* most recently created / front window at head */
static window_t *focused;
static int next_id = 1;
static int wm_dirty;

/* drag state */
static int dragging = 0;
static int drag_dx = 0, drag_dy = 0;
static window_t *drag_win = NULL;

/* mouse cursor */
static u32 cur_x, cur_y;
static int cursor_on = 1;

/* back buffer to avoid a blank flash while repainting */
static u32 *backbuf;
static u64 real_fb_addr;
static u32 real_fb_pitch;

void wm_set_bg(u32 color) { c_desktop = color; }
void wm_set_accent(u32 color) { c_title = color; }

void wm_init(void)
{
    c_desktop      = RGB(0x1a, 0x1e, 0x2b);
    c_taskbar      = RGB(0x24, 0x2a, 0x3b);
    c_tasktext     = RGB(0xa8, 0xb2, 0xc8);
    c_winbg        = RGB(0x1e, 0x24, 0x34);
    c_title        = RGB(0x3f, 0x9a, 0xff);
    c_title_inactive = RGB(0x3a, 0x42, 0x56);
    c_border       = RGB(0x0e, 0x12, 0x1c);
    c_titletext    = RGB(0xe8, 0xec, 0xf2);
    c_close        = RGB(0xe0, 0x4b, 0x4b);
    c_close_hover  = RGB(0xff, 0x6a, 0x6a);
    head = NULL;
    focused = NULL;
    cur_x = 512; cur_y = 384;

    /* allocate a full-screen back buffer (avoids a blank flash on repaint) */
    backbuf = (u32 *)malloc((size_t)g_fb.width * g_fb.height * 4);
}

static void set_client_geom(window_t *w)
{
    w->cx = w->x + WINDOW_BORDER;
    w->cy = w->y + TITLE_BAR_H;
    w->cw = w->w - 2 * WINDOW_BORDER;
    w->ch = w->h - TITLE_BAR_H - WINDOW_BORDER;
}

window_t *wm_create_window(int x, int y, int w, int h, const char *title,
                           win_draw_fn draw, win_key_fn key, win_mouse_fn mouse,
                           win_close_fn close)
{
    window_t *win = (window_t *)malloc(sizeof(window_t));
    if (!win) return NULL;
    memset((void *)win, 0, sizeof(window_t));
    win->id = next_id++;
    win->x = x; win->y = y; win->w = w; win->h = h;
    win->visible = 1;
    if (title) strncpy(win->title, title, 63);
    win->draw = draw;
    win->key = key;
    win->mouse = mouse;
    win->close = close;
    set_client_geom(win);

    /* insert at head (front) */
    win->next = head;
    head = win;
    wm_focus(win);
    return win;
}

void wm_raise(window_t *w)
{
    if (head == w) return;
    window_t **pp = &head;
    while (*pp && *pp != w) pp = &(*pp)->next;
    if (!*pp) return;
    *pp = w->next;
    w->next = head;
    head = w;
}

void wm_focus(window_t *w)
{
    window_t *it;
    if (!w) return;
    for (it = head; it; it = it->next) it->focused = 0;
    w->focused = 1;
    focused = w;
    wm_raise(w);
}

window_t *wm_focused(void) { return focused; }

void wm_destroy_window(window_t *w)
{
    window_t **pp = &head;
    while (*pp && *pp != w) pp = &(*pp)->next;
    if (*pp) *pp = w->next;
    if (focused == w) focused = NULL;
    if (w->close) w->close(w);
    free(w);
}

void wm_invalidate(window_t *w)
{
    (void)w;
    wm_dirty = 1;
}

int wm_is_dirty(void)
{
    return wm_dirty;
}

void wm_clear_dirty(void)
{
    wm_dirty = 0;
}

static int point_in(int x, int y, int rx, int ry, int rw, int rh)
{
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

static int is_close_button(window_t *w, int mx, int my)
{
    /* close button is a ~18x18 square at top-right of title bar */
    int bx = w->x + w->w - 22;
    int by = w->y + 2;
    return point_in(mx, my, bx, by, 18, TITLE_BAR_H - 4);
}

static void draw_close_button(window_t *w, int hover)
{
    int bx = w->x + w->w - 22;
    int by = w->y + 2;
    fb_fill_rect(bx, by, 18, TITLE_BAR_H - 4, hover ? c_close_hover : c_close);
    /* draw an "x" as two diagonals */
    u32 xc = RGB(0xff, 0xff, 0xff);
    int i;
    for (i = 0; i < 5; i++) {
        fb_put_pixel(bx + 5 + i, by + 5 + i, xc);
        fb_put_pixel(bx + 9 - i, by + 5 + i, xc);
    }
}

void wm_draw_titlebar(window_t *w)
{
    u32 tc = w->focused ? c_title : c_title_inactive;
    fb_fill_rect(w->x, w->y, w->w, TITLE_BAR_H, tc);
    fb_draw_text(w->x + 8, w->y + 3, w->title, c_titletext, tc);
    draw_close_button(w, 0);
}

void wm_draw_window_frame(window_t *w)
{
    fb_fill_rect(w->x, w->y, w->w, w->h, c_border);
    wm_draw_titlebar(w);
    fb_fill_rect(w->cx, w->cy, w->cw, w->ch, c_winbg);
    if (w->draw)
        w->draw(w);
}

static void draw_taskbar(void)
{
    int th = 28;
    fb_fill_rect(0, g_fb.height - th, g_fb.width, th, c_taskbar);
    fb_fill_rect(0, g_fb.height - th, g_fb.width, 2, c_title);
    fb_draw_text(8, g_fb.height - th + 6, "SlopOS", c_titletext, c_taskbar);
    /* clock */
    char buf[32];
    u64 secs = timer_seconds();
    ksprintf(buf, sizeof(buf), "uptime %llus", secs);
    int tw = (int)strlen(buf) * 8;
    fb_draw_text(g_fb.width - tw - 12, g_fb.height - th + 6, buf, c_tasktext, c_taskbar);
}

static void draw_cursor(void)
{
    /* simple arrow cursor */
    int i;
    for (i = 0; i < 8; i++) {
        fb_fill_rect(cur_x, cur_y + i, i + 1, 1, RGB(0x00, 0x00, 0x00));
        fb_fill_rect(cur_x + 1, cur_y + i, i - 1 > 0 ? i - 1 : 0, 1, RGB(0xff, 0xff, 0xff));
    }
    fb_fill_rect(cur_x, cur_y + 8, 8, 1, RGB(0x00, 0x00, 0x00));
    fb_fill_rect(cur_x + 6, cur_y + 9, 2, 6, RGB(0xff, 0xff, 0xff));
    fb_fill_rect(cur_x + 6, cur_y + 9, 1, 6, RGB(0x00, 0x00, 0x00));
}

void wm_redraw(void)
{
    /* draw into a back buffer, then blit once (avoids a blank flash) */
    real_fb_addr = g_fb.addr;
    real_fb_pitch = g_fb.pitch;
    if (backbuf) {
        g_fb.addr = (u64)backbuf;
        g_fb.pitch = g_fb.width * 4;
    }

    fb_clear(c_desktop);

    /* draw windows from back (tail) to front (head) */
    /* collect windows into array in reverse order */
    window_t *arr[WM_MAX_WINDOWS];
    int n = 0;
    window_t *w;
    for (w = head; w; w = w->next) {
        if (w->visible && n < WM_MAX_WINDOWS)
            arr[n++] = w;
    }
    for (int i = n - 1; i >= 0; i--)
        wm_draw_window_frame(arr[i]);

    draw_taskbar();
    draw_cursor();

    /* blit the back buffer to the real framebuffer */
    if (backbuf) {
        memcpy((void *)real_fb_addr, (void *)backbuf, (size_t)g_fb.height * g_fb.width * 4);
        g_fb.addr = real_fb_addr;
        g_fb.pitch = real_fb_pitch;
    }
}

void wm_handle_mouse(mouse_state_t *ms)
{
    if (ms->buttons & MOUSE_BTN_LEFT) {
        if (!dragging) {
            /* find topmost window under cursor */
            window_t *w;
            int clicked = 0;
            for (w = head; w; w = w->next) {
                if (!w->visible) continue;
                if (point_in(ms->x, ms->y, w->x, w->y, w->w, w->h)) {
                    wm_focus(w);
                    if (is_close_button(w, ms->x, ms->y)) {
                        wm_destroy_window(w);
                    } else if (ms->y < w->y + TITLE_BAR_H) {
                        /* start dragging */
                        dragging = 1;
                        drag_win = w;
                        drag_dx = ms->x - w->x;
                        drag_dy = ms->y - w->y;
                    } else if (w->mouse) {
                        w->mouse(w, ms);
                    }
                    clicked = 1;
                    break;
                }
            }
            if (!clicked && focused) {
                focused->focused = 0;
                focused = NULL;
            }
        }
    } else {
        if (dragging) {
            dragging = 0;
            drag_win = NULL;
        }
    }

    /* dragging */
    if (dragging && drag_win) {
        int nx = ms->x - drag_dx;
        int ny = ms->y - drag_dy;
        if (nx < 0) nx = 0;
        if (ny < 0) ny = 0;
        if (nx + drag_win->w > (int)g_fb.width) nx = g_fb.width - drag_win->w;
        if (ny + drag_win->h > (int)g_fb.height) ny = g_fb.height - drag_win->h;
        drag_win->x = nx;
        drag_win->y = ny;
        set_client_geom(drag_win);
    }

    /* update cursor position */
    cur_x = ms->x;
    cur_y = ms->y;
    wm_redraw();
}

void wm_handle_key(struct key_event ev)
{
    window_t *w = focused;
    if (w && w->key)
        w->key(w, ev);
}
