/* SlopOS user program: compute primes using mmap + write.
 * Static, freestanding, Linux x86-64 ABI. */
typedef unsigned long u64;
typedef unsigned char u8;

static long syscall3(long n, long a, long b, long c)
{
    long ret;
    register long rax asm("rax") = n;
    register long rdi asm("rdi") = a;
    register long rsi asm("rsi") = b;
    register long rdx asm("rdx") = c;
    asm volatile("syscall" : "=a"(ret)
                 : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx)
                 : "rcx", "r11", "memory");
    return ret;
}

static void write_str(const char *s)
{
    long len = 0;
    while (s[len]) len++;
    syscall3(1, 1, (long)s, len);
}

static void write_num(u64 n)
{
    char buf[24];
    int i = 0;
    if (n == 0) { write_str("0"); return; }
    while (n) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i) { char c = buf[--i]; syscall3(1, 1, (long)&c, 1); }
}

void _start(void)
{
    int N = 1000;
    /* mmap a sieve buffer (addr=0 -> kernel picks, len=N) */
    u8 *sieve = (u8 *)syscall3(9, 0, (long)N, 3);   /* PROT_READ|WRITE */
    if ((long)sieve <= 0) {
        write_str("mmap failed\n");
        syscall3(60, 1, 0, 0);
    }
    for (int i = 0; i < N; i++) sieve[i] = 1;
    sieve[0] = sieve[1] = 0;
    for (int i = 2; i * i < N; i++)
        if (sieve[i])
            for (int j = i * i; j < N; j += i)
                sieve[j] = 0;
    int count = 0;
    for (int i = 2; i < N; i++)
        if (sieve[i]) count++;
    write_str("Primes below 1000: ");
    write_num((u64)count);
    write_str("\nFirst 20: ");
    int shown = 0;
    for (int i = 2; i < N && shown < 20; i++) {
        if (sieve[i]) {
            write_num((u64)i);
            write_str(shown == 19 ? "\n" : " ");
            shown++;
        }
    }
    /* free the mmap region (munmap) */
    syscall3(11, (long)sieve, (long)N, 0);
    syscall3(60, 0, 0, 0);   /* exit(0) */
    for (;;) { }
}
