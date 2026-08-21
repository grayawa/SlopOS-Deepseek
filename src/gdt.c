#include "gdt.h"
#include "lib.h"

struct tss {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
} __attribute__((packed));

static struct tss tss;

/* GDT: 8 descriptor slots (6 code/data + 2 for the 16-byte TSS descriptor) */
static u64 gdt[8] __attribute__((aligned(16)));
static u16 gdt_limit;

/* Build a code/data descriptor */
static u64 make_desc(u32 base, u32 limit, u8 access, u8 flags)
{
    u64 d = 0;
    d |= (u64)(limit & 0xFFFF);             /* limit low 16 */
    d |= (u64)(base & 0xFFFF) << 16;        /* base low 16 */
    d |= (u64)((base >> 16) & 0xFF) << 32;  /* base mid 8 */
    d |= (u64)access << 40;                 /* access byte */
    d |= (u64)((limit >> 16) & 0xF) << 48;  /* limit high 4 */
    d |= (u64)(flags & 0xF) << 52;          /* flags (G, D, L, AVL) */
    d |= (u64)((base >> 24) & 0xFF) << 56;  /* base high 8 */
    return d;
}

/* Build the 16-byte TSS descriptor into two 64-bit slots */
static void make_tss_desc(u64 *lo, u64 *hi, u64 base, u32 limit)
{
    *lo = 0;
    *lo |= (u64)(limit & 0xFFFF);
    *lo |= (u64)(base & 0xFFFF) << 16;
    *lo |= (u64)((base >> 16) & 0xFF) << 32;
    *lo |= (u64)0x89 << 40;                 /* present, 64-bit available TSS */
    *lo |= (u64)((limit >> 16) & 0xF) << 48;
    *lo |= (u64)((base >> 24) & 0xFF) << 56;

    *hi = 0;
    *hi |= (u64)(base >> 32);               /* base bits 32..63 */
    /* bits 64..95 are reserved (0) */
}

void gdt_init(void)
{
    memset((void *)gdt, 0, sizeof(gdt));

    gdt[0] = make_desc(0, 0, 0, 0);                                  /* null */
    gdt[1] = make_desc(0, 0xFFFFFFFF, 0x9A, 0xA);                    /* 0x08 kernel code, L=1 */
    gdt[2] = make_desc(0, 0xFFFFFFFF, 0x92, 0xC);                    /* 0x10 kernel data */
    gdt[3] = make_desc(0, 0xFFFFFFFF, 0xFA, 0xA);                    /* 0x18 user code, DPL=3 */
    gdt[4] = make_desc(0, 0xFFFFFFFF, 0xF2, 0xC);                    /* 0x20 user data, DPL=3 */

    /* TSS at index 5 (selector 0x28) */
    memset((void *)&tss, 0, sizeof(tss));
    tss.iomap_base = sizeof(tss);
    make_tss_desc(&gdt[5], &gdt[6], (u64)&tss, sizeof(tss) - 1);

    gdt_limit = (u16)(sizeof(gdt) - 1);

    /* Load the GDT (10-byte pointer for 64-bit mode) */
    struct {
        u16 limit;
        u64 base;
    } __attribute__((packed)) gdtr = { gdt_limit, (u64)gdt };
    __asm__ volatile ("lgdt %0" : : "m"(gdtr));

    /* Reload segment registers */
    __asm__ volatile ("mov $0x10, %%ax; mov %%ax, %%ds; mov %%ax, %%es; mov %%ax, %%ss; mov %%ax, %%fs; mov %%ax, %%gs" : : : "memory");

    /* Reload CS via far return */
    __asm__ volatile (
        "pushq $0x08\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        : : : "rax", "memory");

    /* Load the TSS */
    __asm__ volatile ("ltr %%ax" : : "a"(GDT_TSS));
}

void gdt_set_tss_rsp0(u64 rsp0)
{
    tss.rsp0 = rsp0;
}

void gdt_set_tss_ist(u64 ist, u64 stack)
{
    if (ist < 7)
        tss.ist[ist] = stack;
}
