#include "terminal.h"
#include "fb.h"
#include "lib.h"
#include "kmalloc.h"
#include "timer.h"
#include "pmm.h"
#include "syscall.h"
#include "program.h"
#include "printk.h"

static const char *prompt_str = "slopos> ";
static terminal_t *console_term;

static void console_write_to_term(const char *buf, size_t len)
{
    if (console_term) {
        size_t i;
        for (i = 0; i < len; i++)
            terminal_putc(console_term, buf[i]);
    }
}

void terminal_bind_console(terminal_t *t)
{
    console_term = t;
    console_set_writer(console_write_to_term);
}

void terminal_run_command(terminal_t *t, const char *cmd)
{
    while (*cmd == ' ') cmd++;
    if (strncmp(cmd, "run ", 4) == 0)
        cmd += 4;
    if (*cmd == '\0') {
        terminal_println(t, "usage: run <program>");
        return;
    }
    terminal_println(t, "Running user program...");
    program_run(cmd);
    terminal_println(t, "Program exited.");
}

void terminal_set_prompt(const char *p)
{
    prompt_str = p;
}

/* push a new (empty) line; scroll buffer if full */
static void new_line(terminal_t *t)
{
    t->cur_line++;
    t->cur_col = 0;
    if (t->cur_line >= TERM_MAXLINES) {
        /* drop the oldest line */
        memmove((void *)t->lines, (void *)t->lines[1], (TERM_MAXLINES - 1) * (TERM_COLS + 1));
        t->cur_line = TERM_MAXLINES - 1;
    }
    t->lines[t->cur_line][0] = '\0';
    if (t->cur_line + 1 > t->nlines)
        t->nlines = t->cur_line + 1;
}

void terminal_putc(terminal_t *t, char c)
{
    if (c == '\n') {
        new_line(t);
    } else if (c == '\r') {
        t->cur_col = 0;
    } else if (c == '\b') {
        if (t->cur_col > 0) {
            t->cur_col--;
            t->lines[t->cur_line][t->cur_col] = '\0';
        }
    } else if (c >= 0x20 && c < 0x7F) {
        int col = t->cur_col;
        if (col < t->cols) {
            t->lines[t->cur_line][col] = c;
            t->lines[t->cur_line][col + 1] = '\0';
            t->cur_col++;
        } else {
            new_line(t);
            t->lines[t->cur_line][0] = c;
            t->lines[t->cur_line][1] = '\0';
            t->cur_col = 1;
        }
    }
    t->dirty = 1;
    wm_invalidate(t->win);
}

void terminal_print(terminal_t *t, const char *s)
{
    while (*s)
        terminal_putc(t, *s++);
}

void terminal_println(terminal_t *t, const char *s)
{
    terminal_print(t, s);
    terminal_putc(t, '\n');
}

void terminal_clear(terminal_t *t)
{
    t->nlines = 0;
    t->cur_line = 0;
    t->cur_col = 0;
    t->lines[0][0] = '\0';
    t->dirty = 1;
    wm_invalidate(t->win);
}

/* ---- shell ---- */
static void shell_exec(terminal_t *t, const char *cmd)
{
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') {
        terminal_print(t, prompt_str);
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        terminal_println(t, "SlopOS built-in commands:");
        terminal_println(t, "  help       show this help");
        terminal_println(t, "  clear      clear the terminal");
        terminal_println(t, "  echo TEXT  print TEXT");
        terminal_println(t, "  uptime     show system uptime");
        terminal_println(t, "  info       show system information");
        terminal_println(t, "  about      about SlopOS");
        terminal_println(t, "  run PROG   run a user program (hello, primes)");
    } else if (strcmp(cmd, "clear") == 0) {
        terminal_clear(t);
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        terminal_println(t, cmd + 5);
    } else if (strcmp(cmd, "uptime") == 0) {
        char buf[64];
        ksprintf(buf, sizeof(buf), "Uptime: %llus (%llu ticks)", timer_seconds(), timer_ticks());
        terminal_println(t, buf);
    } else if (strcmp(cmd, "info") == 0) {
        char buf[128];
        ksprintf(buf, sizeof(buf), "SlopOS x86-64 | %ux%u framebuffer | %u MiB RAM",
                 g_fb.width, g_fb.height, (u32)(pmm_total_mem() >> 20));
        terminal_println(t, buf);
        terminal_println(t, "  CPU: long mode, 4-level paging");
        terminal_println(t, "  Syscalls: Linux-compatible (in progress)");
    } else if (strcmp(cmd, "about") == 0) {
        terminal_println(t, "SlopOS - a from-scratch x86-64 operating system.");
        terminal_println(t, "Licensed 0BSD. Not derived from any existing OS.");
    } else if (strcmp(cmd, "run") == 0 || strncmp(cmd, "run ", 4) == 0) {
        terminal_run_command(t, cmd);
    } else {
        terminal_print(t, "command not found: ");
        terminal_println(t, cmd);
    }
    terminal_print(t, prompt_str);
}

static void term_draw(window_t *w)
{
    terminal_t *t = (terminal_t *)w->user;
    t->cols = w->cw / 8;
    t->rows = w->ch / 8;
    if (t->cols > TERM_COLS) t->cols = TERM_COLS;

    /* fill client background */
    fb_fill_rect(w->cx, w->cy, w->cw, w->ch, RGB(0x14, 0x18, 0x24));

    int start = t->nlines > t->rows ? t->nlines - t->rows : 0;
    int i;
    for (i = 0; i < t->rows; i++) {
        int li = start + i;
        if (li < t->nlines) {
            fb_draw_text(w->cx, w->cy + i * 8, t->lines[li], RGB(0xd8, 0xde, 0xe9), RGB(0x14, 0x18, 0x24));
        }
    }
    /* cursor */
    int cr = t->cur_line - start;
    if (cr >= 0 && cr < t->rows) {
        int cx = w->cx + t->cur_col * 8;
        int cy = w->cy + cr * 8;
        fb_fill_rect(cx, cy, 8, 8, RGB(0x3f, 0x9a, 0xff));
        /* draw the character at the cursor on top */
        char ch = t->lines[t->cur_line][t->cur_col];
        if (ch >= 0x20 && ch < 0x7F)
            fb_draw_char(cx, cy, ch, RGB(0x00, 0x00, 0x00), RGB(0x3f, 0x9a, 0xff));
    }
}

static void term_key(window_t *w, struct key_event ev)
{
    terminal_t *t = (terminal_t *)w->user;
    if (!ev.press) return;
    char c = (char)ev.ascii;
    if (c == '\n') {
        terminal_putc(t, '\n');
        char cmd[TERM_COLS + 1];
        strncpy(cmd, t->cmd, TERM_COLS);
        cmd[TERM_COLS] = '\0';
        t->cmd_len = 0;
        t->cmd[0] = '\0';
        shell_exec(t, cmd);
    } else if (c == '\b') {
        if (t->cmd_len > 0) {
            t->cmd_len--;
            t->cmd[t->cmd_len] = '\0';
            terminal_putc(t, '\b');
        }
    } else if (c >= 0x20 && c < 0x7F) {
        if (t->cmd_len < TERM_COLS) {
            t->cmd[t->cmd_len++] = c;
            t->cmd[t->cmd_len] = '\0';
            terminal_putc(t, c);
        }
    }
}

terminal_t *terminal_create(int x, int y, int w, int h, const char *title)
{
    terminal_t *t = (terminal_t *)malloc(sizeof(terminal_t));
    if (!t) return NULL;
    memset((void *)t, 0, sizeof(terminal_t));
    t->win = wm_create_window(x, y, w, h, title, term_draw, term_key, NULL, NULL);
    t->win->user = t;
    t->cols = (w - 2) / 8;
    t->rows = (h - TITLE_BAR_H - 1) / 8;
    t->lines[0][0] = '\0';
    t->nlines = 1;
    t->cur_line = 0;
    t->cur_col = 0;
    terminal_print(t, "SlopOS terminal. Type 'help' for commands.\n");
    terminal_print(t, prompt_str);
    return t;
}
