#include "printk.h"
#include "serial.h"
#include "lib.h"

void kputc(char c)
{
    serial_putc(c);
}

void kputs(const char *s)
{
    serial_write(s);
}

static void serial_out(char c, void *ctx)
{
    (void)ctx;
    serial_putc(c);
}

static void out_char(kputc_fn out, void *ctx, char c)
{
    if (out)
        out(c, ctx);
}

void kformat(kputc_fn out, void *ctx, const char *fmt, __builtin_va_list ap)
{
    const char *p = fmt;
    char buf[32];

    while (*p) {
        if (*p != '%') {
            out_char(out, ctx, *p++);
            continue;
        }
        p++;
        /* flags */
        int left = 0, zero = 0, plus = 0, space = 0, alt = 0;
        for (;; p++) {
            if (*p == '-') left = 1;
            else if (*p == '0') zero = 1;
            else if (*p == '+') plus = 1;
            else if (*p == ' ') space = 1;
            else if (*p == '#') alt = 1;
            else break;
        }
        int width = 0;
        while (*p >= '0' && *p <= '9')
            width = width * 10 + (*p++ - '0');
        int prec = -1;
        if (*p == '.') {
            p++;
            prec = 0;
            while (*p >= '0' && *p <= '9')
                prec = prec * 10 + (*p++ - '0');
        }
        char len = 0;
        if (*p == 'l') { len = 'l'; p++; if (*p == 'l') { len = 'L'; p++; } }
        else if (*p == 'h') { len = 'h'; p++; }

        char spec = *p++;
        char pad = zero ? '0' : ' ';

        /* %s */
        if (spec == 's') {
            const char *s = __builtin_va_arg(ap, const char *);
            if (!s) s = "(null)";
            int n = (int)strlen(s);
            if (prec >= 0 && n > prec) n = prec;
            if (!left) for (int i = 0; i < width - n; i++) out_char(out, ctx, ' ');
            for (int i = 0; i < n; i++) out_char(out, ctx, s[i]);
            if (left) for (int i = 0; i < width - n; i++) out_char(out, ctx, ' ');
            continue;
        }
        if (spec == 'c') {
            char c = (char)__builtin_va_arg(ap, int);
            out_char(out, ctx, c);
            continue;
        }
        if (spec == '%') { out_char(out, ctx, '%'); continue; }

        /* numeric */
        i64 sval = 0;
        u64 uval = 0;
        int base = 10;
        int is_neg = 0;
        int uppercase = 0;

        if (spec == 'd' || spec == 'i') {
            if (len == 'L') sval = __builtin_va_arg(ap, i64);
            else if (len == 'l') sval = __builtin_va_arg(ap, long);
            else sval = __builtin_va_arg(ap, int);
            if (sval < 0) { is_neg = 1; uval = (u64)(-sval); }
            else uval = (u64)sval;
            base = 10;
        } else if (spec == 'u') {
            if (len == 'L') uval = __builtin_va_arg(ap, u64);
            else if (len == 'l') uval = __builtin_va_arg(ap, unsigned long);
            else uval = __builtin_va_arg(ap, unsigned int);
            base = 10;
        } else if (spec == 'x' || spec == 'X') {
            if (len == 'L') uval = __builtin_va_arg(ap, u64);
            else if (len == 'l') uval = __builtin_va_arg(ap, unsigned long);
            else uval = __builtin_va_arg(ap, unsigned int);
            base = 16;
            uppercase = (spec == 'X');
        } else if (spec == 'o') {
            if (len == 'L') uval = __builtin_va_arg(ap, u64);
            else if (len == 'l') uval = __builtin_va_arg(ap, unsigned long);
            else uval = __builtin_va_arg(ap, unsigned int);
            base = 8;
        } else if (spec == 'p') {
            uval = __builtin_va_arg(ap, u64);
            base = 16;
            alt = 1;
            pad = '0';
            if (width == 0) width = 16;
        } else {
            out_char(out, ctx, '%');
            out_char(out, ctx, spec);
            continue;
        }

        static const char *digits = "0123456789abcdef";
        static const char *DIGITS = "0123456789ABCDEF";
        const char *dg = uppercase ? DIGITS : digits;
        int idx = 0;
        if (uval == 0) buf[idx++] = '0';
        while (uval > 0) {
            buf[idx++] = dg[uval % base];
            uval /= base;
        }
        int numlen = idx;
        int prefix = 0;
        if (alt && base == 16 && numlen > 0) prefix = 2;
        if (alt && base == 8 && (numlen == 0 || buf[idx-1] != '0')) prefix = 1;
        int sign = 0;
        if (is_neg) sign = 1;
        else if (plus) sign = 1;
        else if (space) sign = 2;

        int total = sign + prefix + numlen;
        if (!left) {
            for (int i = 0; i < width - total; i++) out_char(out, ctx, pad);
        }
        if (is_neg) out_char(out, ctx, '-');
        else if (plus) out_char(out, ctx, '+');
        else if (space) out_char(out, ctx, ' ');
        if (prefix) {
            if (base == 16) { out_char(out, ctx, '0'); out_char(out, ctx, uppercase ? 'X' : 'x'); }
            else out_char(out, ctx, '0');
        }
        for (int i = numlen - 1; i >= 0; i--) out_char(out, ctx, buf[i]);
        if (left) for (int i = 0; i < width - total; i++) out_char(out, ctx, ' ');
    }
}

void kprintf(const char *fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    kformat(serial_out, NULL, fmt, ap);
    __builtin_va_end(ap);
}

/* buffer sink for ksprintf */
struct buf_sink {
    char *buf;
    size_t size;
    size_t len;
};
static void buf_out(char c, void *ctx)
{
    struct buf_sink *s = (struct buf_sink *)ctx;
    if (s->len + 1 < s->size)
        s->buf[s->len] = c;
    s->len++;
}

int ksprintf(char *buf, size_t size, const char *fmt, ...)
{
    struct buf_sink s;
    s.buf = buf;
    s.size = size;
    s.len = 0;
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    kformat(buf_out, &s, fmt, ap);
    __builtin_va_end(ap);
    if (size > 0)
        buf[s.len < size ? s.len : size - 1] = '\0';
    return (int)s.len;
}
