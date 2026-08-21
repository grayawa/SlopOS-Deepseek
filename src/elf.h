#ifndef SLOPOS_ELF_H
#define SLOPOS_ELF_H

#include "types.h"

/* Load an ELF64 image into the given page table (pml4).
 * Returns the entry point on success, 0 on failure. */
u64 elf_load(const u8 *image, size_t size, u64 pml4);

/* Set up a fresh user address space (pml4) and user stack.
 * Returns the pml4 physical address, 0 on failure. */
u64 elf_create_pml4(u64 *user_stack_top_out);

/* Fill the initial user stack with argc/argv/envp (Linux ABI).
 * Returns the stack pointer to set in the task. */
u64 elf_setup_stack(u64 user_stack_top, int argc, char **argv);

#endif
