#include "kmalloc.h"
#include "pmm.h"
#include "lib.h"

#define ALIGN 16
#define POOL_SIZE (64 * 1024)

typedef struct blk {
    size_t size;            /* usable payload size (excluding this header) */
    struct blk *next;       /* free-list link (only meaningful when free) */
} blk_t;

static blk_t *freelist = NULL;

void kmalloc_init(void)
{
    /* nothing pre-allocated; pools are added on demand */
}

/* allocate fresh pages from pmm and add them to the free list */
static void add_pool(size_t len)
{
    size_t pages = (len + 4095) / 4096;
    u64 phys = pmm_alloc_contiguous((u32)pages);
    if (!phys) return;
    blk_t *b = (blk_t *)(u64)phys;
    b->size = pages * 4096 - sizeof(blk_t);
    /* insert in sorted order by address */
    blk_t **pp = &freelist;
    while (*pp && (u8 *)*pp < (u8 *)b)
        pp = &(*pp)->next;
    b->next = *pp;
    *pp = b;
    /* coalesce with predecessor if adjacent */
    if (pp != &freelist) {
        blk_t *prev = NULL;
        for (blk_t *it = freelist; it && it->next != b; it = it->next)
            prev = it;
        if (prev && (u8 *)prev + sizeof(blk_t) + prev->size == (u8 *)b) {
            prev->size += sizeof(blk_t) + b->size;
            prev->next = b->next;
            b = prev;
        }
    }
}

void *malloc(size_t n)
{
    if (n == 0) n = 1;
    n = (n + ALIGN - 1) & ~((size_t)ALIGN - 1);

    blk_t **pp = &freelist;
    while (*pp) {
        blk_t *b = *pp;
        if (b->size >= n) {
            /* split if the remainder can hold a header + a useful chunk */
            if (b->size >= n + sizeof(blk_t) + 16) {
                blk_t *nb = (blk_t *)((u8 *)b + sizeof(blk_t) + n);
                nb->size = b->size - n - sizeof(blk_t);
                nb->next = b->next;
                *pp = nb;
                b->size = n;
            } else {
                *pp = b->next;
            }
            return (void *)((u8 *)b + sizeof(blk_t));
        }
        pp = &b->next;
    }

    add_pool(n + POOL_SIZE);
    return malloc(n);
}

void free(void *ptr)
{
    if (!ptr) return;
    blk_t *b = (blk_t *)((u8 *)ptr - sizeof(blk_t));
    /* insert sorted by address */
    blk_t **pp = &freelist;
    while (*pp && (u8 *)*pp < (u8 *)b)
        pp = &(*pp)->next;
    b->next = *pp;
    *pp = b;

    /* coalesce with next */
    if (b->next && (u8 *)b + sizeof(blk_t) + b->size == (u8 *)b->next) {
        b->size += sizeof(blk_t) + b->next->size;
        b->next = b->next->next;
    }
    /* coalesce with previous */
    if (pp != &freelist) {
        blk_t *prev = NULL;
        for (blk_t *it = freelist; it && it->next != b; it = it->next)
            prev = it;
        if (prev && (u8 *)prev + sizeof(blk_t) + prev->size == (u8 *)b) {
            prev->size += sizeof(blk_t) + b->size;
            prev->next = b->next;
        }
    }
}

void *calloc(size_t n, size_t sz)
{
    size_t total = n * sz;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *p, size_t n)
{
    if (!p) return malloc(n);
    if (n == 0) { free(p); return NULL; }
    blk_t *b = (blk_t *)((u8 *)p - sizeof(blk_t));
    if (b->size >= n) return p;
    void *np = malloc(n);
    if (!np) return NULL;
    memcpy(np, p, b->size);
    free(p);
    return np;
}
