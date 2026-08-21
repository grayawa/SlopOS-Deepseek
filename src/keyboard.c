#include "keyboard.h"
#include "port.h"
#include "printk.h"
#include "lib.h"

#define KBD_CMD_PORT 0x64
#define KBD_DATA_PORT 0x60

#define EVENT_QUEUE 256

/* scancode set 1 -> ASCII (unshifted) */
static const char ascii_map[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0, 'a','s',
    'd','f','g','h','j','k','l',';','\'','`', 0, '\\','z','x','c','v',
    'b','n','m',',','.','/', 0,'*', 0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* shifted ASCII for printable keys */
static const char ascii_map_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0, 'A','S',
    'D','F','G','H','J','K','L',':','"','~', 0, '|','Z','X','C','V',
    'B','N','M','<','>','?', 0,'*', 0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static struct key_event queue[EVENT_QUEUE];
static volatile u16 q_head, q_tail;
static key_handler_t handler;
static u8 modifiers;
static u8 extended;    /* previous byte was 0xE0 */

static void ps2_wait_write(void)
{
    while (inb(KBD_CMD_PORT) & 0x02)
        ;
}
static void ps2_wait_read(void)
{
    while (!(inb(KBD_CMD_PORT) & 0x01))
        ;
}

static void enqueue(struct key_event ev)
{
    u16 next = (q_tail + 1) % EVENT_QUEUE;
    if (next == q_head)
        return;   /* full */
    queue[q_tail] = ev;
    q_tail = next;
    if (handler)
        handler(ev);
}

void kbd_set_handler(key_handler_t h)
{
    handler = h;
}

int kbd_get_event(struct key_event *ev)
{
    if (q_head == q_tail)
        return 0;
    *ev = queue[q_head];
    q_head = (q_head + 1) % EVENT_QUEUE;
    return 1;
}

static void handle_byte(u8 sc)
{
    struct key_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.scancode = sc;

    if (sc == 0xE0) {
        extended = 1;
        return;
    }
    if (extended) {
        extended = 0;
        /* extended scancodes: ignore for now (arrows, etc.) */
        return;
    }

    if (sc == 0x2A || sc == 0x36) { modifiers |= KEY_MOD_SHIFT; return; }
    if (sc == 0xAA || sc == 0xB6) { modifiers &= ~KEY_MOD_SHIFT; return; }
    if (sc == 0x1D) { modifiers |= KEY_MOD_CTRL; return; }
    if (sc == 0x9D) { modifiers &= ~KEY_MOD_CTRL; return; }
    if (sc == 0x38) { modifiers |= KEY_MOD_ALT; return; }
    if (sc == 0xB8) { modifiers &= ~KEY_MOD_ALT; return; }
    if (sc == 0x3A) { modifiers ^= KEY_MOD_CAPS; return; }

    ev.press = (sc < 0x80);
    u8 idx = sc & 0x7F;
    if (ev.press) {
        u8 c = ascii_map[idx];
        u8 cs = ascii_map_shift[idx];
        if (modifiers & KEY_MOD_SHIFT)
            c = cs;
        if ((modifiers & KEY_MOD_CAPS) && c >= 'a' && c <= 'z')
            c = c - 'a' + 'A';
        ev.ascii = c;
    }
    ev.modifiers = modifiers;
    enqueue(ev);
}

static void kbd_irq(isr_frame_t *frame)
{
    (void)frame;
    u8 status = inb(KBD_CMD_PORT);
    if (status & 0x01) {
        u8 sc = inb(KBD_DATA_PORT);
        handle_byte(sc);
    }
}

void kbd_init(void)
{
    /* enable keyboard (command 0xAE), set default scancode set */
    ps2_wait_write();
    outb(KBD_CMD_PORT, 0xAE);
    /* read and set config byte: enable IRQ1 (keyboard) */
    ps2_wait_write();
    outb(KBD_CMD_PORT, 0x20);
    ps2_wait_read();
    u8 cfg = inb(KBD_DATA_PORT);
    cfg |= 0x01;               /* enable IRQ1 */
    ps2_wait_write();
    outb(KBD_CMD_PORT, 0x60);
    ps2_wait_write();
    outb(KBD_DATA_PORT, cfg);

    irq_register_handler(1, kbd_irq);
    kprintf("[kbd] initialized\n");
}
