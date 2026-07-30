/* SlopOS GDT Implementation
 * SPDX-License-Identifier: 0BSD
 */
#include "gdt.h"

static struct gdt_entry gdt[7];
static struct gdt_ptr gdt_ptr;
static struct tss tss;

void gdt_init(void) {
    /* Null descriptor */
    gdt[0].limit_low = 0;
    gdt[0].base_low = 0;
    gdt[0].base_mid = 0;
    gdt[0].access = 0;
    gdt[0].granularity = 0;
    gdt[0].base_high = 0;

    /* Kernel code 64-bit: 0x08 */
    gdt[1].limit_low = 0;
    gdt[1].base_low = 0;
    gdt[1].base_mid = 0;
    gdt[1].access = 0x9A;
    gdt[1].granularity = 0x20;
    gdt[1].base_high = 0;

    /* Kernel data: 0x10 */
    gdt[2].limit_low = 0;
    gdt[2].base_low = 0;
    gdt[2].base_mid = 0;
    gdt[2].access = 0x92;
    gdt[2].granularity = 0;
    gdt[2].base_high = 0;

    /* User code 64-bit: 0x18 */
    gdt[3].limit_low = 0;
    gdt[3].base_low = 0;
    gdt[3].base_mid = 0;
    gdt[3].access = 0xFA;
    gdt[3].granularity = 0x20;
    gdt[3].base_high = 0;

    /* User data: 0x20 */
    gdt[4].limit_low = 0;
    gdt[4].base_low = 0;
    gdt[4].base_mid = 0;
    gdt[4].access = 0xF2;
    gdt[4].granularity = 0;
    gdt[4].base_high = 0;

    /* TSS: 0x28 */
    uint64_t tss_base = (uint64_t)&tss;
    uint64_t tss_limit = sizeof(struct tss) - 1;
    gdt[5].limit_low = tss_limit & 0xFFFF;
    gdt[5].base_low = tss_base & 0xFFFF;
    gdt[5].base_mid = (tss_base >> 16) & 0xFF;
    gdt[5].access = 0x89;
    gdt[5].granularity = 0;
    gdt[5].base_high = (tss_base >> 24) & 0xFF;

    /* TSS upper 32 bits */
    gdt[6].limit_low = 0;
    gdt[6].base_low = (tss_base >> 32) & 0xFFFF;
    gdt[6].base_mid = (tss_base >> 48) & 0xFF;
    gdt[6].access = 0;
    gdt[6].granularity = 0;
    gdt[6].base_high = 0;

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;

    __asm__ volatile(
        "lgdt %0\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x2B, %%ax\n"
        "ltr %%ax\n"
        : : "m"(gdt_ptr) : "rax", "memory"
    );
}

void tss_set_stack(uint64_t stack) {
    tss.rsp[0] = stack;
}
