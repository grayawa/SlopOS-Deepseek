#ifndef SLOPOS_SERIAL_H
#define SLOPOS_SERIAL_H

#include "types.h"

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);
void serial_hex(u64 value);
void serial_dec(u64 value);

#endif
