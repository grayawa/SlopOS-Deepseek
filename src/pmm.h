#ifndef SLOPOS_PMM_H
#define SLOPOS_PMM_H

#include "types.h"

/* Physical memory manager: 4 KiB frames, bitmap allocator. */
void pmm_init(u64 multiboot_info_addr);
u64  pmm_alloc(void);              /* returns physical address of one frame, 0 on failure */
void pmm_free(u64 phys);
u64  pmm_alloc_contiguous(u32 count); /* returns physical address of `count` consecutive frames */
u64  pmm_total_mem(void);          /* bytes of usable RAM detected */
void pmm_mark_reserved(u64 base, u64 len);

#endif
