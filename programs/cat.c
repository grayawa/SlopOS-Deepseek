/* SlopOS user program: cat - open, read, and print a file.
 * Static, freestanding, Linux x86-64 ABI. */
typedef unsigned long u64;

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

static long syscall1(long n, long a)
{
    return syscall3(n, a, 0, 0);
}

void _start(void)
{
    const char *path = "/readme.txt";
    char buf[256];

    long fd = syscall3(2, (long)path, 0, 0);   /* open(path, O_RDONLY) */
    if (fd < 0) {
        const char *e = "cat: cannot open file\n";
        long i = 0; while (e[i]) i++;
        syscall3(1, 2, (long)e, i);
        syscall3(60, 1, 0, 0);
    }
    long n;
    while ((n = syscall3(0, fd, (long)buf, (long)sizeof(buf))) > 0) {
        syscall3(1, 1, (long)buf, n);
    }
    syscall1(3, fd);       /* close(fd) */
    syscall3(60, 0, 0, 0); /* exit(0) */
    for (;;) { }
}
