#ifndef SLOPOS_GDT_H
#define SLOPOS_GDT_H

#include "types.h"

/* GDT selectors */
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18
#define GDT_USER_DATA   0x20
#define GDT_TSS         0x28

void gdt_init(void);
void gdt_set_tss_rsp0(u64 rsp0);
void gdt_set_tss_ist(u64 ist, u64 stack);

#endif
