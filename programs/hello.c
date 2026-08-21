/* A minimal SlopOS user program.
 * Uses raw Linux x86-64 syscalls (write, exit) so it runs unmodified
 * on a Linux-compatible syscall ABI. Compiled as a static, freestanding
 * ELF64 with no libc. */
typedef unsigned long u64;

static long syscall3(long n, long a, long b, long c)
{
    long ret;
    register long rax asm("rax") = n;
    register long rdi asm("rdi") = a;
    register long rsi asm("rsi") = b;
    register long rdx asm("rdx") = c;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx)
                 : "rcx", "r11", "memory");
    return ret;
}

void _start(void)
{
    const char *msg = "Hello from SlopOS user mode!\n";
    long len = 0;
    while (msg[len]) len++;
    syscall3(1, 1, (long)msg, len);   /* write(1, msg, len) */
    syscall3(1, 2, (long)msg, len);   /* write(2, msg, len) */
    syscall3(60, 0, 0, 0);            /* exit(0) */
    for (;;) { }
}
