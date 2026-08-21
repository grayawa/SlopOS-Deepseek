#ifndef SLOPOS_PROGRAM_H
#define SLOPOS_PROGRAM_H

#include "types.h"

/* Register a multiboot module (an ELF image) by name. */
void program_register(const char *name, u64 addr, u64 size);
/* Load and run a registered user program; returns when it exits. */
int program_run(const char *name);

#endif
