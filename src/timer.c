#include "timer.h"
#include "port.h"

static volatile u64 ticks;
static volatile u32 hz;
static void (*callback)(void);

static void timer_irq(isr_frame_t *frame)
{
    (void)frame;
    ticks++;
    if (callback)
        callback();
}

void timer_init(u32 hz_)
{
    hz = hz_;
    u32 divisor = PIT_FREQ / hz_;
    outb(0x43, 0x36);                 /* channel 0, lobyte/hibyte, mode 2, binary */
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    irq_register_handler(0, timer_irq);
}

u64 timer_ticks(void)
{
    return ticks;
}

void timer_sleep_ms(u32 ms)
{
    u64 target = ticks + (u64)((u64)ms * hz / 1000);
    while (ticks < target)
        __asm__ volatile ("hlt");
}

void timer_set_callback(void (*cb)(void))
{
    callback = cb;
}
