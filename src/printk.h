#ifndef SLOPOS_PRINTK_H
#define SLOPOS_PRINTK_H

#include "types.h"

void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...);
/* output a formatted string to an arbitrary sink (used by terminal later) */
typedef void (*kputc_fn)(char c, void *ctx);
void kformat(kputc_fn out, void *ctx, const char *fmt, __builtin_va_list ap);
int  ksprintf(char *buf, size_t size, const char *fmt, ...);

#endif
