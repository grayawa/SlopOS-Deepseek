#ifndef SLOPOS_TERMINAL_H
#define SLOPOS_TERMINAL_H

#include "types.h"
#include "wm.h"

#define TERM_MAXLINES 512
#define TERM_COLS     200

typedef struct {
    window_t *win;
    int rows, cols;              /* in character cells */
    char lines[TERM_MAXLINES][TERM_COLS + 1];
    int nlines;                  /* total lines in the buffer */
    int cur_line;                /* index of the active (input) line */
    int cur_col;
    char cmd[TERM_COLS + 1];     /* current input line buffer */
    int cmd_len;
    int dirty;
} terminal_t;

terminal_t *terminal_create(int x, int y, int w, int h, const char *title);
void terminal_putc(terminal_t *t, char c);
void terminal_print(terminal_t *t, const char *s);
void terminal_println(terminal_t *t, const char *s);
void terminal_clear(terminal_t *t);
void terminal_set_prompt(const char *p);

#endif
