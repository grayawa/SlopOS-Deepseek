#include "types.h"
#include "serial.h"
#include "fb.h"
#include "printk.h"
#include "lib.h"
#include "port.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "pmm.h"
#include "vmm.h"
#include "keyboard.h"
#include "mouse.h"

/* ---- multiboot2 info parsing ---- */
#define MB2_TAG_END         0
#define MB2_TAG_FRAMEBUFFER 8

struct mb2_tag {
    u32 type;
    u32 size;
};

struct mb2_info {
    u32 total_size;
    u32 reserved;
};

static u32 parse_framebuffer(u64 info_addr)
{
    struct mb2_info *info = (struct mb2_info *)info_addr;
    u64 ptr = info_addr + 8;
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
            kprintf("  framebuffer: %ux%u pitch=%u bpp=%u\n", width, height, pitch, bpp);
            return 1;
        }
        ptr += (tag->size + 7) & ~7ULL;
    }
    return 0;
}

/* ---- desktop colors ---- */
static u32 c_bg, c_taskbar, c_text, c_accent, c_cursor;

static void draw_desktop(void)
{
    fb_clear(c_bg);
    /* taskbar */
    fb_fill_rect(0, g_fb.height - 28, g_fb.width, 28, c_taskbar);
    fb_fill_rect(0, g_fb.height - 28, g_fb.width, 2, c_accent);
    fb_draw_text(8, g_fb.height - 22, "SlopOS Desktop", c_text, c_taskbar);
    fb_draw_text(g_fb.width - 150, g_fb.height - 22, "x86-64 | input test", c_text, c_taskbar);
}

void kernel_main(u32 magic, u32 info_addr)
{
    serial_init();
    serial_write("\n==== SlopOS boot ====\n");
    kprintf("  magic=0x%llx info=0x%llx\n", (u64)magic, (u64)info_addr);

    u32 fb_ok = parse_framebuffer((u64)info_addr);
    if (!fb_ok || !g_fb.mapped) {
        kputs("  ERROR: no framebuffer\n");
        for (;;) hlt();
    }

    /* init CPU + subsystems */
    gdt_init();
    kputs("  gdt ok\n");
    idt_init();
    kputs("  idt ok\n");
    pmm_init((u64)info_addr);
    vmm_init();
    timer_init(100);
    kbd_init();
    mouse_init();
    sti();

    c_bg      = RGB(0x1a, 0x1e, 0x2b);
    c_taskbar = RGB(0x24, 0x2a, 0x3b);
    c_text    = RGB(0xd8, 0xde, 0xe9);
    c_accent  = RGB(0x3f, 0x9a, 0xff);
    c_cursor  = RGB(0xe8, 0xec, 0xf2);

    draw_desktop();

    u64 last_print = 0;
    struct key_event kev;
    static u32 log_y = 8;
    static u32 log_x = 8;
    u32 px = g_fb.width / 2, py = g_fb.height / 2;
    int cursor_valid = 0;

    kprintf("  boot complete\n");

    for (;;) {
        /* keyboard */
        while (kbd_get_event(&kev)) {
            if (kev.press) {
                if (kev.ascii == '\n') {
                    log_y += 16;
                    log_x = 8;
                } else if (kev.ascii == '\b') {
                    if (log_x > 8) log_x -= 8;
                } else if (kev.ascii >= 32) {
                    fb_draw_char(log_x, log_y, (char)kev.ascii, c_text, c_bg);
                    log_x += 8;
                }
                if (log_y > g_fb.height - 60) {
                    log_y = 8;
                    /* clear log region */
                    fb_fill_rect(0, 8, g_fb.width, g_fb.height - 60, c_bg);
                }
            }
        }

        /* mouse cursor */
        mouse_state_t ms;
        mouse_get_state(&ms);
        if (cursor_valid) {
            fb_fill_rect(px, py, 12, 16, c_bg);
        }
        px = ms.x; py = ms.y;
        fb_fill_rect(px, py, 12, 16, c_cursor);
        fb_fill_rect(px, py, 12, 2, c_accent);
        cursor_valid = 1;

        /* timer tick report */
        if (timer_ticks() - last_print >= 100) {
            last_print = timer_ticks();
            kprintf("[timer] ticks=%llu\n", timer_ticks());
        }

        __asm__ volatile ("hlt");
    }
}
