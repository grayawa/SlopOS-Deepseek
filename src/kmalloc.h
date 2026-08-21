#ifndef SLOPOS_KMALLOC_H
#define SLOPOS_KMALLOC_H

#include "types.h"

void *malloc(size_t n);
void *calloc(size_t n, size_t sz);
void *realloc(void *p, size_t n);
void free(void *p);
void kmalloc_init(void);

#endif
