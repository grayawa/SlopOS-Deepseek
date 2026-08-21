#include "fb.h"
#include "font8x16.h"

framebuffer_t g_fb;

void fb_init(u64 addr, u32 pitch, u32 width, u32 height, u8 bpp)
{
    g_fb.addr   = addr;
    g_fb.pitch  = pitch;
    g_fb.width  = width;
    g_fb.height = height;
    g_fb.bpp    = bpp;
    g_fb.type   = FB_PIXEL_FORMAT_RGB;
    g_fb.mapped = 1;

    /* Default 32-bit RGB layout (mask sizes/positions as seen with QEMU + GRUB). */
    g_fb.red_mask_size = 8;   g_fb.red_field_pos = 16;
    g_fb.green_mask_size = 8; g_fb.green_field_pos = 8;
    g_fb.blue_mask_size = 8;  g_fb.blue_field_pos = 0;
}

u32 fb_pixel(u8 r, u8 g, u8 b)
{
    /* 32-bit: 0x00RRGGBB */
    if (g_fb.bpp == 32)
        return ((u32)r << 16) | ((u32)g << 8) | (u32)b;
    if (g_fb.bpp == 24)
        return ((u32)r << 16) | ((u32)g << 8) | (u32)b;
    return 0;
}

static inline u32 *pixel_ptr(u32 x, u32 y)
{
    return (u32 *)(u64)(g_fb.addr + (u64)y * g_fb.pitch + (u64)x * (g_fb.bpp / 8));
}

void fb_put_pixel(u32 x, u32 y, u32 color)
{
    if (x >= g_fb.width || y >= g_fb.height)
        return;
    *pixel_ptr(x, y) = color;
}

void fb_set_pixel_raw(u32 x, u32 y, u32 color)
{
    fb_put_pixel(x, y, color);
}

void fb_fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color)
{
    u32 i, j;
    for (j = y; j < y + h && j < g_fb.height; j++) {
        for (i = x; i < x + w && i < g_fb.width; i++)
            *pixel_ptr(i, j) = color;
    }
}

void fb_clear(u32 color)
{
    fb_fill_rect(0, 0, g_fb.width, g_fb.height, color);
}

void fb_draw_char(u32 x, u32 y, char c, u32 fg, u32 bg)
{
    u32 row, col;
    const u8 *glyph;
    if ((u8)c < 0x20 || (u8)c > 0x7E)
        c = '?';
    glyph = font8x16[(u8)c - 0x20];
    for (row = 0; row < 16; row++) {
        u8 bits = glyph[row];
        for (col = 0; col < 8; col++) {
            u32 color = (bits & (0x80 >> col)) ? fg : bg;
            fb_put_pixel(x + col, y + row, color);
        }
    }
}

void fb_draw_text(u32 x, u32 y, const char *s, u32 fg, u32 bg)
{
    while (*s) {
        fb_draw_char(x, y, *s++, fg, bg);
        x += 8;
    }
}

void fb_draw_text_transparent(u32 x, u32 y, const char *s, u32 fg)
{
    u32 row, col;
    while (*s) {
        const u8 *glyph;
        char c = *s++;
        if ((u8)c < 0x20 || (u8)c > 0x7E)
            c = '?';
        glyph = font8x16[(u8)c - 0x20];
        for (row = 0; row < 16; row++) {
            u8 bits = glyph[row];
            for (col = 0; col < 8; col++) {
                if (bits & (0x80 >> col))
                    fb_put_pixel(x + col, y + row, fg);
            }
        }
        x += 8;
    }
}

void fb_blit(u32 x, u32 y, const u32 *pixels, u32 w, u32 h)
{
    u32 i, j;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++)
            fb_put_pixel(x + i, y + j, pixels[j * w + i]);
    }
}
