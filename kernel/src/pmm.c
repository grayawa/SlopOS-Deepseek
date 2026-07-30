/* SlopOS Physical Memory Manager
 * SPDX-License-Identifier: 0BSD
 */
#include "pmm.h"
#include "../limine.h"
#include "serial.h"

static uint8_t *bitmap = NULL;
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t bitmap_pages = 0;
static uint64_t first_free_page = 0;
static uint64_t mem_low = 0;
static uint64_t mem_high = 0;

void pmm_init(struct limine_memmap_response *memmap) {
    if (!memmap) {
        serial_write_str("[PMM] No memory map, cannot init!\n");
        return;
    }

    /* Find the highest usable memory address */
    uint64_t highest = 0;
    uint64_t lowest = UINT64_MAX;
    uint64_t usable_total = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE) {
            uint64_t end = e->base + e->length;
            if (end > highest) highest = end;
            if (e->base < lowest) lowest = e->base;
            usable_total += e->length;
        }
    }

    mem_low = lowest;
    mem_high = highest;
    total_pages = highest / PAGE_SIZE;
    bitmap_pages = (total_pages + 7) / 8;
    bitmap_pages = (bitmap_pages + PAGE_SIZE - 1) / PAGE_SIZE;

    serial_write_str("[PMM] Total usable: ");
    {
        char buf[32]; uint64_t mb = usable_total / (1024*1024);
        int i = 0;
        if (mb == 0) { serial_write_str("0"); }
        else {
            char tmp[20]; int j = 0;
            while (mb) { tmp[j++] = '0' + (mb % 10); mb /= 10; }
            while (j--) serial_write(tmp[j]);
        }
    }
    serial_write_str(" MB\n");

    /* Find a place for the bitmap in usable memory */
    uint64_t bitmap_base = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE && e->length >= bitmap_pages * PAGE_SIZE) {
            bitmap_base = e->base;
            break;
        }
    }

    if (!bitmap_base) {
        serial_write_str("[PMM] Cannot find space for bitmap!\n");
        return;
    }

    extern uint64_t hhdm_offset;
    bitmap = (uint8_t *)(bitmap_base + hhdm_offset);

    /* Mark all pages as used initially */
    for (uint64_t i = 0; i < bitmap_pages * PAGE_SIZE; i++) {
        bitmap[i] = 0xFF;
    }

    /* Mark usable pages as free */
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_page = e->base / PAGE_SIZE;
            uint64_t end_page = (e->base + e->length) / PAGE_SIZE;
            for (uint64_t p = start_page; p < end_page && p < total_pages; p++) {
                bitmap[p / 8] &= ~(1 << (p % 8));
                free_pages++;
            }
        }
    }

    /* Mark bitmap pages as used */
    uint64_t bp_start = bitmap_base / PAGE_SIZE;
    for (uint64_t p = bp_start; p < bp_start + bitmap_pages; p++) {
        bitmap[p / 8] |= (1 << (p % 8));
        free_pages--;
    }

    first_free_page = 0;
    serial_write_str("[PMM] Initialized. Free: ");
    {
        uint64_t mb = (free_pages * PAGE_SIZE) / (1024*1024);
        char tmp[20]; int j = 0;
        if (mb == 0) { serial_write_str("0"); }
        else {
            while (mb) { tmp[j++] = '0' + (mb % 10); mb /= 10; }
            while (j--) serial_write(tmp[j]);
        }
    }
    serial_write_str(" MB\n");
}

void *pmm_alloc_page(void) {
    if (!bitmap || free_pages == 0) return NULL;

    for (uint64_t i = first_free_page; i < total_pages; i++) {
        if (!(bitmap[i / 8] & (1 << (i % 8)))) {
            bitmap[i / 8] |= (1 << (i % 8));
            free_pages--;
            first_free_page = i + 1;
            extern uint64_t hhdm_offset;
            return (void *)(i * PAGE_SIZE + hhdm_offset);
        }
    }

    /* Wrap around */
    for (uint64_t i = 0; i < first_free_page; i++) {
        if (!(bitmap[i / 8] & (1 << (i % 8)))) {
            bitmap[i / 8] |= (1 << (i % 8));
            free_pages--;
            first_free_page = i + 1;
            extern uint64_t hhdm_offset;
            return (void *)(i * PAGE_SIZE + hhdm_offset);
        }
    }

    return NULL;
}

void *pmm_alloc_pages(size_t count) {
    if (!bitmap || free_pages < count) return NULL;
    if (count == 0) return NULL;

    for (uint64_t i = 0; i < total_pages - count; i++) {
        int found = 1;
        for (uint64_t j = 0; j < count; j++) {
            if (bitmap[(i + j) / 8] & (1 << ((i + j) % 8))) {
                found = 0;
                i += j;
                break;
            }
        }
        if (found) {
            for (uint64_t j = 0; j < count; j++) {
                bitmap[(i + j) / 8] |= (1 << ((i + j) % 8));
            }
            free_pages -= count;
            extern uint64_t hhdm_offset;
            return (void *)(i * PAGE_SIZE + hhdm_offset);
        }
    }
    return NULL;
}

void pmm_free_page(void *addr) {
    if (!bitmap) return;
    extern uint64_t hhdm_offset;
    uint64_t page = ((uint64_t)addr - hhdm_offset) / PAGE_SIZE;
    if (page >= total_pages) return;
    if (bitmap[page / 8] & (1 << (page % 8))) {
        bitmap[page / 8] &= ~(1 << (page % 8));
        free_pages++;
        if (page < first_free_page) first_free_page = page;
    }
}

void pmm_free_pages(void *addr, size_t count) {
    if (!bitmap) return;
    extern uint64_t hhdm_offset;
    uint64_t page = ((uint64_t)addr - hhdm_offset) / PAGE_SIZE;
    for (size_t i = 0; i < count; i++) {
        uint64_t p = page + i;
        if (p >= total_pages) break;
        if (bitmap[p / 8] & (1 << (p % 8))) {
            bitmap[p / 8] &= ~(1 << (p % 8));
            free_pages++;
            if (p < first_free_page) first_free_page = p;
        }
    }
}

uint64_t pmm_total_memory(void) { return total_pages * PAGE_SIZE; }
uint64_t pmm_free_memory(void) { return free_pages * PAGE_SIZE; }
uint64_t pmm_used_memory(void) { return (total_pages - free_pages) * PAGE_SIZE; }
