#include "pmm.h"
#include "lib.h"
#include "printk.h"
#include "fb.h"

#define FRAME_SIZE 4096ULL
#define FRAME_MASK ~(FRAME_SIZE - 1)

/* linker symbols */
extern u64 __bss_start;
extern u64 __bss_end;

static u32 *bitmap;
static u64 total_frames;
static u64 total_mem;
static u64 last_alloc;

static inline void bm_set(u64 frame)
{
    bitmap[frame >> 5] |= (1u << (frame & 31));
}
static inline void bm_clear(u64 frame)
{
    bitmap[frame >> 5] &= ~(1u << (frame & 31));
}
static inline int bm_get(u64 frame)
{
    return (bitmap[frame >> 5] >> (frame & 31)) & 1;
}

/* reserve [base, base+len) */
static void reserve_range(u64 base, u64 len)
{
    u64 start = (base + FRAME_SIZE - 1) & FRAME_MASK;
    u64 end = (base + len) & FRAME_MASK;
    u64 f;
    for (f = start >> 12; f < (end >> 12); f++)
        if (f < total_frames)
            bm_set(f);
}

/* free [base, base+len) */
static void free_range(u64 base, u64 len)
{
    u64 start = base & FRAME_MASK;
    u64 end = (base + len) & FRAME_MASK;
    u64 f;
    for (f = start >> 12; f < (end >> 12); f++)
        if (f < total_frames)
            bm_clear(f);
}

void pmm_mark_reserved(u64 base, u64 len)
{
    reserve_range(base, len);
}

void pmm_init(u64 info_addr)
{
    /* find highest usable address from multiboot2 mmap */
    u32 *info = (u32 *)info_addr;
    u64 total_size = info[0];
    u64 ptr = info_addr + 8;
    u64 end = info_addr + total_size;
    u64 max_usable = 0;

    while (ptr + 8 <= end) {
        u32 type = *(u32 *)ptr;
        u32 size = *(u32 *)(ptr + 4);
        if (type == 0) break;
        if (type == 6 && size >= 20) {  /* mmap */
            u8 *d = (u8 *)ptr;
            u32 entry_size = *(u32 *)(d + 8);
            u32 n = (size - 16) / entry_size;
            u32 i;
            for (i = 0; i < n; i++) {
                u8 *e = d + 16 + i * entry_size;
                u64 base = *(u64 *)e;
                u64 len = *(u64 *)(e + 8);
                u32 etype = *(u32 *)(e + 16);
                if (etype == 1) {  /* available */
                    u64 top = base + len;
                    if (top > max_usable)
                        max_usable = top;
                }
            }
        }
        ptr += (size + 7) & ~7ULL;
    }

    total_mem = max_usable;
    total_frames = max_usable / FRAME_SIZE;
    if (max_usable & (FRAME_SIZE - 1))
        total_frames++;

    /* place the bitmap right after the kernel's BSS */
    u64 bm_start = ((u64)&__bss_end + FRAME_SIZE - 1) & FRAME_MASK;
    bitmap = (u32 *)bm_start;
    u64 bm_bytes = ((total_frames + 31) / 32) * 4;
    memset((void *)bm_start, 0xFF, bm_bytes);   /* all frames reserved initially */

    /* mark bitmap region reserved (it is, since 0xFF) */
    reserve_range(bm_start, bm_bytes);

    /* free usable ranges from mmap, then re-reserve special regions */
    ptr = info_addr + 8;
    while (ptr + 8 <= end) {
        u32 type = *(u32 *)ptr;
        u32 size = *(u32 *)(ptr + 4);
        if (type == 0) break;
        if (type == 6 && size >= 20) {
            u8 *d = (u8 *)ptr;
            u32 entry_size = *(u32 *)(d + 8);
            u32 n = (size - 16) / entry_size;
            u32 i;
            for (i = 0; i < n; i++) {
                u8 *e = d + 16 + i * entry_size;
                u64 base = *(u64 *)e;
                u64 len = *(u64 *)(e + 8);
                u32 etype = *(u32 *)(e + 16);
                if (etype == 1)
                    free_range(base, len);
            }
        }
        ptr += (size + 7) & ~7ULL;
    }

    /* re-reserve critical regions */
    reserve_range(0, 0x100000);               /* low memory (BIOS, VGA, IVT) */
    reserve_range((u64)&__bss_start, ((u64)&__bss_end) - (u64)&__bss_start); /* kernel+bss */
    reserve_range(bm_start, bm_bytes);         /* bitmap */
    if (g_fb.mapped)
        reserve_range(g_fb.addr, (u64)g_fb.pitch * g_fb.height);  /* framebuffer */

    last_alloc = 0x100000;

    kprintf("[pmm] %u MiB usable, %u frames\n", (u32)(total_mem >> 20), (u32)total_frames);
}

u64 pmm_alloc(void)
{
    u64 i;
    for (i = last_alloc >> 12; i < total_frames; i++) {
        if (!bm_get(i)) {
            bm_set(i);
            last_alloc = (i + 1) << 12;
            return i << 12;
        }
    }
    for (i = 0; i < (last_alloc >> 12); i++) {
        if (!bm_get(i)) {
            bm_set(i);
            last_alloc = (i + 1) << 12;
            return i << 12;
        }
    }
    return 0;
}

void pmm_free(u64 phys)
{
    u64 frame = phys >> 12;
    if (frame < total_frames)
        bm_clear(frame);
}

u64 pmm_alloc_contiguous(u32 count)
{
    u64 i, j;
    if (count == 0) return 0;
    for (i = last_alloc >> 12; i + count <= total_frames; i++) {
        int ok = 1;
        for (j = 0; j < count; j++) {
            if (bm_get(i + j)) { ok = 0; break; }
        }
        if (ok) {
            for (j = 0; j < count; j++) bm_set(i + j);
            last_alloc = (i + count) << 12;
            return i << 12;
        }
    }
    for (i = 0; i + count <= (last_alloc >> 12); i++) {
        int ok = 1;
        for (j = 0; j < count; j++) {
            if (bm_get(i + j)) { ok = 0; break; }
        }
        if (ok) {
            for (j = 0; j < count; j++) bm_set(i + j);
            return i << 12;
        }
    }
    return 0;
}

u64 pmm_total_mem(void)
{
    return total_mem;
}
