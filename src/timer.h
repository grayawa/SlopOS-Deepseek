#ifndef SLOPOS_TIMER_H
#define SLOPOS_TIMER_H

#include "types.h"
#include "idt.h"

#define PIT_FREQ 1193182

void timer_init(u32 hz);
u64 timer_ticks(void);
void timer_sleep_ms(u32 ms);
void timer_set_callback(void (*cb)(void));

#endif
