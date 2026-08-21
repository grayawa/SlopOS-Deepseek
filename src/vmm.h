#ifndef SLOPOS_VMM_H
#define SLOPOS_VMM_H

#include "types.h"

/* page table entry flags */
#define PTE_P    (1ULL << 0)
#define PTE_W    (1ULL << 1)
#define PTE_U    (1ULL << 2)
#define PTE_PS   (1ULL << 7)
#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ULL

void vmm_init(void);
u64  vmm_kernel_pml4(void);
u64  vmm_create_pml4(void);
void vmm_free_pml4(u64 pml4);
void vmm_switch(u64 pml4);
int  vmm_map_page(u64 pml4, u64 virt, u64 phys, u64 flags);
int  vmm_alloc_page(u64 pml4, u64 virt, u64 flags);
int  vmm_map_2mb(u64 pml4, u64 virt, u64 phys, u64 flags);
int  vmm_unmap_page(u64 pml4, u64 virt);
u64  vmm_virt_to_phys(u64 pml4, u64 virt);
/* map a contiguous range (n pages) */
int  vmm_map_pages(u64 pml4, u64 virt, u64 phys, u64 n, u64 flags);
int  vmm_alloc_pages(u64 pml4, u64 virt, u64 n, u64 flags);

#endif
