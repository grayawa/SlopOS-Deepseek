#ifndef SLOPOS_FB_H
#define SLOPOS_FB_H

#include "types.h"

#define FB_PIXEL_FORMAT_RGB 1

typedef struct {
    u64  addr;       /* linear framebuffer physical address */
    u32  pitch;      /* bytes per scanline */
    u32  width;
    u32  height;
    u8   bpp;        /* bits per pixel */
    u8   type;       /* 0=indexed, 1=RGB */
    u8   red_mask_size, red_field_pos;
    u8   green_mask_size, green_field_pos;
    u8   blue_mask_size, blue_field_pos;
    u32  mapped;     /* whether the framebuffer is usable */
} framebuffer_t;

extern framebuffer_t g_fb;

void fb_init(u64 addr, u32 pitch, u32 width, u32 height, u8 bpp);
u32  fb_pixel(u8 r, u8 g, u8 b);
void fb_put_pixel(u32 x, u32 y, u32 color);
void fb_fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color);
void fb_clear(u32 color);
void fb_draw_char(u32 x, u32 y, char c, u32 fg, u32 bg);
void fb_draw_text(u32 x, u32 y, const char *s, u32 fg, u32 bg);
void fb_draw_text_transparent(u32 x, u32 y, const char *s, u32 fg);
void fb_blit(u32 x, u32 y, const u32 *pixels, u32 w, u32 h);
void fb_set_pixel_raw(u32 x, u32 y, u32 color);

/* color helpers */
#define RGB(r,g,b) fb_pixel((r),(g),(b))

#endif
