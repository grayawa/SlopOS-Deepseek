#include "types.h"
#include "serial.h"
#include "fb.h"

/* ---- multiboot2 info parsing ---- */
#define MB2_TAG_END         0
#define MB2_TAG_CMDLINE     1
#define MB2_TAG_BOOTLOADER  2
#define MB2_TAG_MODULE      3
#define MB2_TAG_BASIC_MEM   4
#define MB2_TAG_MMAP        6
#define MB2_TAG_FRAMEBUFFER 8

struct mb2_tag {
    u32 type;
    u32 size;
    /* data follows, aligned to 8 bytes */
};

struct mb2_info {
    u32 total_size;
    u32 reserved;
    /* tags follow */
};

static u32 parse_framebuffer(u64 info_addr)
{
    struct mb2_info *info = (struct mb2_info *)info_addr;
    u64 ptr = info_addr + 8;                    /* skip total_size + reserved */
    u64 end = info_addr + info->total_size;

    while (ptr + 8 <= end) {
        struct mb2_tag *tag = (struct mb2_tag *)ptr;
        if (tag->type == MB2_TAG_END)
            break;
        if (tag->type == MB2_TAG_FRAMEBUFFER && tag->size >= 24) {
            u8 *d = (u8 *)tag;
            u64 addr = *(u64 *)(d + 8);
            u32 pitch = *(u32 *)(d + 16);
            u32 width = *(u32 *)(d + 20);
            u32 height = *(u32 *)(d + 24);
            u8 bpp = d[28];
            fb_init(addr, pitch, width, height, bpp);
            serial_write("  framebuffer: ");
            serial_dec(width);
            serial_write("x");
            serial_dec(height);
            serial_write(" pitch=");
            serial_dec(pitch);
            serial_write(" bpp=");
            serial_dec(bpp);
            serial_write("\n");
            return 1;
        }
        /* advance to next tag, aligned to 8 bytes */
        ptr += (tag->size + 7) & ~7ULL;
    }
    return 0;
}

void kernel_main(u32 magic, u32 info_addr)
{
    serial_init();
    serial_write("\n==== SlopOS boot ====\n");
    serial_write("  magic=");
    serial_hex(magic);
    serial_write(" info=");
    serial_hex(info_addr);
    serial_write("\n");

    u32 fb_ok = parse_framebuffer((u64)info_addr);

    if (!fb_ok || !g_fb.mapped) {
        serial_write("  ERROR: no framebuffer\n");
        for (;;) { __asm__ volatile("hlt"); }
    }

    /* draw boot screen */
    u32 bg      = RGB(0x10, 0x14, 0x22);
    u32 accent  = RGB(0x3f, 0x9a, 0xff);
    u32 accent2 = RGB(0x26, 0xd9, 0x7c);
    u32 white   = RGB(0xe8, 0xec, 0xf2);

    fb_clear(bg);

    /* top accent bar */
    fb_fill_rect(0, 0, g_fb.width, 6, accent);
    fb_fill_rect(0, 6, g_fb.width, 2, accent2);

    /* "SlopOS" wordmark */
    fb_draw_text_transparent(40, 80, "SlopOS", accent);
    fb_draw_text(40, 100, "A from-scratch operating system", white, bg);

    /* three feature blocks */
    fb_fill_rect(40, 160, 280, 120, RGB(0x1a, 0x21, 0x36));
    fb_fill_rect(340, 160, 280, 120, RGB(0x1a, 0x21, 0x36));
    fb_fill_rect(640, 160, 280, 120, RGB(0x1a, 0x21, 0x36));
    fb_draw_text(60, 180, "x86-64 long mode", white, bg);
    fb_draw_text(360, 180, "Multitasking", white, bg);
    fb_draw_text(660, 180, "Linux ABI", white, bg);

    /* status footer */
    fb_draw_text(40, g_fb.height - 40, "Boot OK - framebuffer active", accent2, bg);

    serial_write("  boot complete, entering idle\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}
