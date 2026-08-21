#include "mouse.h"
#include "port.h"
#include "printk.h"
#include "fb.h"

#define PS2_CMD_PORT 0x64
#define PS2_DATA_PORT 0x60

static mouse_state_t state;
static mouse_handler_t handler;

static void ps2_wait_write(void)
{
    while (inb(PS2_CMD_PORT) & 0x02)
        ;
}
static void ps2_wait_read(void)
{
    while (!(inb(PS2_CMD_PORT) & 0x01))
        ;
}

/* send a command to the auxiliary (mouse) device via the data port */
static void ps2_aux_command(u8 cmd)
{
    ps2_wait_write();
    outb(PS2_CMD_PORT, 0xD4);      /* write to aux device */
    ps2_wait_write();
    outb(PS2_DATA_PORT, cmd);
}

/* read a response byte from the aux device */
static u8 ps2_aux_read(void)
{
    ps2_wait_read();
    return inb(PS2_DATA_PORT);
}

static void mouse_irq(isr_frame_t *frame)
{
    (void)frame;
    u8 status = inb(PS2_CMD_PORT);
    if (!(status & 0x01))
        return;
    u8 data = inb(PS2_DATA_PORT);
    /* bit 5 of status indicates data from the aux (mouse) device */
    if (!(status & 0x20))
        return;   /* keyboard data, ignore here */

    static u8 packet[4];
    static int idx = 0;
    packet[idx++] = data;
    if (idx == 3) {
        if (!(packet[0] & 0x08)) {
            /* not a valid packet start, resync */
            idx = 0;
            return;
        }
        int dx = (int)packet[1] - ((packet[0] & 0x10) ? 256 : 0);
        int dy = (int)packet[2] - ((packet[0] & 0x20) ? 256 : 0);
        state.buttons = packet[0] & 0x07;
        state.dx = (i8)dx;
        state.dy = (i8)dy;
        if (state.x + dx < g_fb.width && (int)(state.x + dx) >= 0)
            state.x += dx;
        if (state.y + dy < g_fb.height && (int)(state.y + dy) >= 0)
            state.y += dy;
        idx = 0;
        if (handler)
            handler(&state);
    }
}

void mouse_set_handler(mouse_handler_t h)
{
    handler = h;
}

void mouse_get_state(mouse_state_t *st)
{
    *st = state;
}

void mouse_init(void)
{
    /* enable the auxiliary device */
    ps2_wait_write();
    outb(PS2_CMD_PORT, 0xA8);

    /* read config byte, enable IRQ12 (bit 1) */
    ps2_wait_write();
    outb(PS2_CMD_PORT, 0x20);
    ps2_wait_read();
    u8 cfg = inb(PS2_DATA_PORT);
    cfg |= 0x02;                  /* enable aux IRQ */
    ps2_wait_write();
    outb(PS2_CMD_PORT, 0x60);
    ps2_wait_write();
    outb(PS2_DATA_PORT, cfg);

    /* mouse defaults: F6 (defaults), F4 (enable reporting) */
    ps2_aux_command(0xF6);
    ps2_aux_read();
    ps2_aux_command(0xF4);
    ps2_aux_read();

    state.x = g_fb.width / 2;
    state.y = g_fb.height / 2;

    irq_register_handler(12, mouse_irq);
    kprintf("[mouse] initialized\n");
}
