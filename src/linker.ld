OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    . = 1M;

    .text : ALIGN(4K) {
        *(.multiboot2)
        *(.text*)
        *(.text)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
        *(.rodata)
    }

    .data : ALIGN(4K) {
        *(.data*)
        *(.data)
    }

    .bss : ALIGN(4K) {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        *(.bss)
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note*)
        *(.comment*)
    }
}
