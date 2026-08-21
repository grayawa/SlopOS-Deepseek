#ifndef SLOPOS_PORT_H
#define SLOPOS_PORT_H

#include "types.h"

static inline void outb(u16 port, u8 val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline u8 inb(u16 port)
{
    u8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outw(u16 port, u16 val)
{
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}
static inline u16 inw(u16 port)
{
    u16 ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outl(u16 port, u32 val)
{
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}
static inline u32 inl(u16 port)
{
    u32 ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void io_wait(void)
{
    outb(0x80, 0);
}
static inline void cli(void)
{
    __asm__ volatile ("cli");
}
static inline void sti(void)
{
    __asm__ volatile ("sti");
}
static inline void hlt(void)
{
    __asm__ volatile ("hlt");
}
static inline u64 read_cr2(void)
{
    u64 v;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(v));
    return v;
}
static inline u64 read_cr3(void)
{
    u64 v;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(v));
    return v;
}
static inline void write_cr3(u64 v)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(v));
}
static inline u64 read_rflags(void)
{
    u64 v;
    __asm__ volatile ("pushfq; pop %0" : "=r"(v));
    return v;
}
static inline u64 read_msr(u32 msr)
{
    u32 lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((u64)hi << 32) | lo;
}
static inline void write_msr(u32 msr, u64 val)
{
    u32 lo = (u32)val;
    u32 hi = (u32)(val >> 32);
    __asm__ volatile ("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}
static inline void cpu_relax(void)
{
    __asm__ volatile ("pause");
}
static inline void invlpg(u64 addr)
{
    __asm__ volatile ("invlpg (%0)" : : "r"(addr) : "memory");
}
static inline u64 read_tsc(void)
{
    u32 lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
}

#endif
