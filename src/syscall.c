#include "syscall.h"
#include "task.h"
#include "vmm.h"
#include "pmm.h"
#include "lib.h"
#include "port.h"
#include "serial.h"
#include "printk.h"

/* ---- syscall MSR setup (Linux x86-64 style) ---- */
void syscall_init(void)
{
    /* STAR: kernel CS in bits 47:32, user CS in bits 63:48 */
    write_msr(0xC0000081, ((u64)0x18 << 48) | ((u64)0x08 << 32));
    /* LSTAR: syscall entry */
    extern void syscall_entry(void);
    write_msr(0xC0000082, (u64)&syscall_entry);
    /* SFMASK: clear IF + DF on syscall */
    write_msr(0xC0000084, 0x600);
    /* enable SYSCALL/SYSRET (EFER.SCE) */
    u64 efer = read_msr(0xC0000080);
    efer |= 1ULL;   /* SCE */
    write_msr(0xC0000080, efer);
}

/* ---- console output routing ---- */
static void (*g_console_write)(const char *buf, size_t len);

void console_set_writer(void (*fn)(const char *buf, size_t len))
{
    g_console_write = fn;
}

void console_write(const char *buf, size_t len)
{
    if (g_console_write)
        g_console_write(buf, len);
    else {
        size_t i;
        for (i = 0; i < len; i++) serial_putc(buf[i]);
    }
}

/* ---- Linux syscall numbers ---- */
#define SYS_READ        0
#define SYS_WRITE       1
#define SYS_OPEN        2
#define SYS_CLOSE       3
#define SYS_MMAP        9
#define SYS_BRK         12
#define SYS_MUNMAP      11
#define SYS_GETPID      39
#define SYS_EXIT        60
#define SYS_EXIT_GROUP  231

/* program break tracking (for brk) */
u64 program_break = 0x800000000ULL;
u64 user_base = 0x400000;

u64 syscall_dispatch(struct syscall_frame *f)
{
    u64 nr = f->rax;
    switch (nr) {
    case SYS_WRITE: {
        u64 fd = f->rdi, buf = f->rsi, len = f->rdx;
        (void)fd;
        console_write((const char *)buf, (size_t)len);
        return len;
    }
    case SYS_READ: {
        /* fd=0: return EOF (0) for now */
        return 0;
    }
    case SYS_OPEN:
        /* not implemented: return -1 */
        return (u64)-1;
    case SYS_CLOSE:
        return 0;
    case SYS_GETPID: {
        task_t *t = sched_current();
        return t ? (u64)t->id : 0;
    }
    case SYS_BRK: {
        u64 new_brk = f->rdi;
        if (new_brk == 0)
            return program_break;
        u64 aligned = (new_brk + 0xFFF) & ~0xFFFULL;
        u64 old_aligned = (program_break + 0xFFF) & ~0xFFFULL;
        if (aligned > old_aligned) {
            u64 pml4 = read_cr3();   /* current (user) address space */
            u64 addr;
            for (addr = old_aligned; addr < aligned; addr += 0x1000)
                vmm_alloc_page(pml4, addr, PTE_W | PTE_U);
        }
        program_break = new_brk;
        return new_brk;
    }
    case SYS_MMAP: {
        u64 addr = f->rdi, len = f->rsi;
        (void)f->rdx; (void)f->r10; (void)f->r8; (void)f->r9;
        if (addr == 0) {
            addr = (program_break + 0xFFF) & ~0xFFFULL;
            program_break = addr + ((len + 0xFFF) & ~0xFFFULL);
        }
        u64 pages = (len + 0xFFF) / 0x1000;
        u64 pml4 = read_cr3();
        u64 i;
        for (i = 0; i < pages; i++)
            vmm_alloc_page(pml4, addr + i * 0x1000, PTE_W | PTE_U);
        return addr;
    }
    case SYS_MUNMAP:
        return 0;
    case SYS_EXIT:
    case SYS_EXIT_GROUP:
        sched_exit(f->rdi);
        return 0;   /* not reached */
    default:
        kprintf("[syscall] unsupported nr=%llu\n", nr);
        return (u64)-1;
    }
}
