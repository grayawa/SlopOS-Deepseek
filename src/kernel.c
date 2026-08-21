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
#include "kmalloc.h"
#include "keyboard.h"
#include "mouse.h"
#include "wm.h"
#include "terminal.h"
#include "task.h"
#include "syscall.h"
#include "program.h"

/* ---- multiboot2 info parsing ---- */
#define MB2_TAG_END         0
#define MB2_TAG_MODULE      3
#define MB2_TAG_FRAMEBUFFER 8

struct mb2_tag {
    u32 type;
    u32 size;
};

struct mb2_info {
    u32 total_size;
    u32 reserved;
};

/* find a multiboot2 module by its command-line name; returns addr/size */
static int find_module(u64 info_addr, const char *name, u64 *addr_out, u64 *size_out)
{
    struct mb2_info *info = (struct mb2_info *)info_addr;
    u64 ptr = info_addr + 8;
    u64 end = info_addr + info->total_size;
    while (ptr + 8 <= end) {
        struct mb2_tag *tag = (struct mb2_tag *)ptr;
        if (tag->type == MB2_TAG_END)
            break;
        if (tag->type == MB2_TAG_MODULE && tag->size >= 16) {
            u8 *d = (u8 *)tag;
            u32 start = *(u32 *)(d + 8);
            u32 m_end = *(u32 *)(d + 12);
            const char *cmdline = (const char *)(d + 16);
            if (cmdline && strcmp(cmdline, name) == 0) {
                *addr_out = start;
                *size_out = (u64)m_end - start;
                return 1;
            }
        }
        ptr += (tag->size + 7) & ~7ULL;
    }
    return 0;
}

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

/* draw callback for the "About SlopOS" window */
static void about_draw(window_t *w)
{
    (void)w;
    u32 bg = RGB(0x14, 0x18, 0x24);
    u32 fg = RGB(0xd8, 0xde, 0xe9);
    u32 ac = RGB(0x3f, 0x9a, 0xff);
    u32 gr = RGB(0x7a, 0x84, 0x99);
    fb_fill_rect(w->cx, w->cy, w->cw, w->ch, bg);
    fb_draw_text(w->cx + 8, w->cy + 8, "SlopOS 0.1", ac, bg);
    fb_draw_text(w->cx + 8, w->cy + 24, "From-scratch x86-64 OS", fg, bg);
    fb_draw_text(w->cx + 8, w->cy + 40, "Boots in QEMU (multiboot2)", gr, bg);
    fb_draw_text(w->cx + 8, w->cy + 56, "Long mode, paging, interrupts", gr, bg);
    fb_draw_text(w->cx + 8, w->cy + 72, "License: 0BSD", gr, bg);
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

    gdt_init();
    idt_init();
    pmm_init((u64)info_addr);
    vmm_init();
    kmalloc_init();
    timer_init(100);
    kbd_init();
    mouse_init();

    wm_init();
    window_t *about = wm_create_window(560, 60, 400, 240, "About SlopOS",
                                      about_draw, NULL, NULL, NULL);
    terminal_t *term = terminal_create(60, 60, 620, 400, "Terminal");
    terminal_bind_console(term);
    (void)term; (void)about;

    sti();
    kprintf("  boot complete - desktop up\n");
    wm_redraw();

    /* ---- set up tasks and run the demo user program ---- */
    sched_init();
    syscall_init();

    u64 mod_addr = 0, mod_size = 0;
    if (find_module((u64)info_addr, "hello", &mod_addr, &mod_size)) {
        program_register("hello", mod_addr, mod_size);
        program_run("hello");
    }
    if (find_module((u64)info_addr, "primes", &mod_addr, &mod_size)) {
        program_register("primes", mod_addr, mod_size);
    }

    u64 last_redraw = 0;
    for (;;) {
        sti();   /* the boot task may resume with IF clear (after a syscall) */
        struct key_event kev;
        while (kbd_get_event(&kev))
            wm_handle_key(kev);

        if (mouse_dirty()) {
            mouse_state_t ms;
            mouse_get_state(&ms);
            wm_handle_mouse(&ms);
            mouse_clear_dirty();
        }

        /* batch repaints: redraw once per input batch or ~1s for the clock */
        if (wm_is_dirty() || timer_ticks() - last_redraw >= 100) {
            last_redraw = timer_ticks();
            wm_redraw();
            wm_clear_dirty();
        }

        __asm__ volatile ("hlt");
    }
}
