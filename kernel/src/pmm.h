/* SlopOS Physical Memory Manager
 * SPDX-License-Identifier: 0BSD
 */
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

struct limine_memmap_response;

void pmm_init(struct limine_memmap_response *memmap);
void *pmm_alloc_page(void);
void *pmm_alloc_pages(size_t count);
void pmm_free_page(void *addr);
void pmm_free_pages(void *addr, size_t count);
uint64_t pmm_total_memory(void);
uint64_t pmm_free_memory(void);
uint64_t pmm_used_memory(void);

#endif
