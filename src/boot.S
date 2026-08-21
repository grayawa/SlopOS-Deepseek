/* SlopOS boot loader entry — multiboot2, x86-64.
 *
 * GRUB loads this kernel as a multiboot2 ELF in 32-bit protected mode.
 * This code builds identity-mapped page tables, enables long mode, and
 * calls the 64-bit C kernel.
 */

/* ---- multiboot2 header ---- */
.set MB_MAGIC, 0xE85250D6
.set MB_ARCH,  0            /* i386 protected mode (we enter long mode ourselves) */

.section .multiboot2, "a"
.align 8
multiboot_header:
    .long MB_MAGIC
    .long MB_ARCH
    .long .Lmh_end - .Lmh_start
    .long -(MB_MAGIC + MB_ARCH + (.Lmh_end - .Lmh_start))
.Lmh_start:
    /* framebuffer request tag (type 5): ask for a linear framebuffer */
    .align 8
    .word 5
    .word 0
    .long 20
    .long 1024               /* width  */
    .long 768                /* height */
    .long 32                 /* depth  */
    .long 0                  /* flags  */
    /* end tag */
    .align 8
    .word 0
    .word 0
    .long 8
.Lmh_end:

/* ---- 32-bit entry ---- */
.section .text
.code32
.global _start
.type _start, @function

/* minimal 16550 serial init + putc for early tracing */
.macro ser_init
    movb $0x00, %al
    movw $0x3F9, %dx
    outb %al, %dx
    movb $0x80, %al
    movw $0x3FB, %dx
    outb %al, %dx
    movb $0x03, %al
    movw $0x3F8, %dx
    outb %al, %dx
    movb $0x00, %al
    movw $0x3F9, %dx
    outb %al, %dx
    movb $0x03, %al
    movw $0x3FB, %dx
    outb %al, %dx
    movb $0xC7, %al
    movw $0x3FA, %dx
    outb %al, %dx
    movb $0x0B, %al
    movw $0x3FC, %dx
    outb %al, %dx
.endm

.macro ser_putc val
    movw $0x3FD, %dx
1:  inb %dx, %al
    testb $0x20, %al
    jz 1b
    movw $0x3F8, %dx
    movb \val, %al
    outb %al, %dx
.endm

_start:
    cli
    cld
    ser_init
    ser_putc $'S'          /* S = entry reached */
    movl %ebx, boot_info_ptr       /* multiboot2 info pointer (physical) */
    movl %eax, boot_magic          /* multiboot2 magic */
    ser_putc $'M'          /* M = saved magic/info */

    /* load our GDT (32-bit pointer form) */
    lgdt gdt32_ptr
    ser_putc $'G'          /* G = GDT loaded */

    /* Build identity-mapped page tables for the first 4 GiB (2 MiB pages). */
    movl $pdpt, %eax
    orl  $0x3, %eax
    movl %eax, pml4                /* pml4[0] -> pdpt */

    movl $pd0, %eax
    orl  $0x3, %eax
    movl %eax, pdpt                /* pdpt[0] -> pd0 */
    movl $pd1, %eax
    orl  $0x3, %eax
    movl %eax, pdpt + 8            /* pdpt[1] -> pd1 */
    movl $pd2, %eax
    orl  $0x3, %eax
    movl %eax, pdpt + 16           /* pdpt[2] -> pd2 */
    movl $pd3, %eax
    orl  $0x3, %eax
    movl %eax, pdpt + 24           /* pdpt[3] -> pd3 */

    /* fill each page directory with 512 x 2 MiB huge pages (base | 0x83) */
    leal pd0, %edi
    movl $0x00000083, %ebx
    call fill_pd
    leal pd1, %edi
    movl $0x40000083, %ebx
    call fill_pd
    leal pd2, %edi
    movl $0x80000083, %ebx
    call fill_pd
    leal pd3, %edi
    movl $0xC0000083, %ebx
    call fill_pd
    ser_putc $'P'          /* P = page tables built */

    /* enable PAE */
    movl %cr4, %eax
    orl  $0x20, %eax
    movl %eax, %cr4
    ser_putc $'A'          /* A = PAE on */

    /* load pml4 -> cr3 */
    movl $pml4, %eax
    movl %eax, %cr3
    ser_putc $'3'          /* 3 = cr3 loaded */

    /* set EFER.LME (MSR 0xC0000080, bit 8) */
    movl $0xC0000080, %ecx
    rdmsr
    orl  $0x100, %eax
    wrmsr
    ser_putc $'E'          /* E = EFER.LME set */

    /* enable paging -> long mode active (still compatibility mode) */
    movl %cr0, %eax
    orl  $0x80000000, %eax
    movl %eax, %cr0
    ser_putc $'L'          /* L = long mode enabled */

    /* far jump into 64-bit code segment (selector 0x08) */
    ljmp $0x08, $long_mode

fill_pd:
    movl $512, %ecx
1:
    movl %ebx, (%edi)
    addl $0x200000, %ebx
    addl $8, %edi
    decl %ecx
    jnz  1b
    ret

/* ---- 64-bit long mode ---- */
.section .text
.code64
long_mode:
    ser_putc $'6'          /* 6 = in 64-bit mode */
    /* set up stack (identity-mapped low memory) */
    leaq stack_top(%rip), %rsp

    /* zero the frame pointer */
    xorq %rbp, %rbp

    /* reload data segment selectors */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs

    /* reload GDT with 64-bit pointer form */
    lgdt gdt64_ptr(%rip)
    ser_putc $'7'          /* 7 = GDT64 loaded */

    /* far-return into the long-mode code segment to refresh CS */
    pushq $0x08
    leaq .Lreload_cs(%rip), %rax
    pushq %rax
    lretq
.Lreload_cs:
    ser_putc $'8'          /* 8 = CS reloaded */
    /* pass multiboot magic + info pointer to the C kernel */
    movl boot_magic(%rip), %edi
    movl boot_info_ptr(%rip), %esi
    call kernel_main
1:
    cli
    hlt
    jmp 1b

/* ---- GDT ---- */
.section .rodata
.align 8
gdt64:
    .quad 0x0000000000000000        /* 0x00 null            */
    .quad 0x00AF9A000000FFFF        /* 0x08 kernel code (L=1) */
    .quad 0x00CF92000000FFFF        /* 0x10 kernel data       */
gdt64_end:
.align 8
gdt32_ptr:
    .word gdt64_end - gdt64 - 1
    .long gdt64
gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .quad gdt64

/* ---- BSS: page tables + boot stack ---- */
.section .bss
.align 4096
.global pml4
.global pdpt
pml4:   .skip 4096
pdpt:   .skip 4096
pd0:    .skip 4096
pd1:    .skip 4096
pd2:    .skip 4096
pd3:    .skip 4096
.align 16
stack_bottom:
    .skip 32768
stack_top:

/* saved multiboot values */
.global boot_magic
.global boot_info_ptr
boot_magic:     .skip 8
boot_info_ptr:  .skip 8

/* mark stack non-executable */
.section .note.GNU-stack,"",@progbits
