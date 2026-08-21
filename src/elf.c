#include "elf.h"
#include "vmm.h"
#include "pmm.h"
#include "lib.h"
#include "printk.h"

#define PT_LOAD 1
#define PF_X 1
#define PF_W 2
#define PF_R 4

struct elf_ehdr {
    u8  e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
};

struct elf_phdr {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
};

u64 elf_create_pml4(u64 *user_stack_top_out)
{
    u64 pml4 = vmm_create_pml4();
    if (!pml4) return 0;

    /* user stack at 0x8000000000, growing down (64 MiB) */
    u64 stack_top = 0x8000000000ULL;
    u64 stack_pages = 64;   /* 256 KiB */
    u64 i;
    u64 sp = stack_top - stack_pages * 0x1000;
    for (i = 0; i < stack_pages; i++) {
        if (vmm_alloc_page(pml4, sp + i * 0x1000, PTE_W | PTE_U) < 0) {
            kprintf("[elf] stack alloc failed\n");
            return 0;
        }
    }
    *user_stack_top_out = stack_top;
    return pml4;
}

u64 elf_load(const u8 *image, size_t size, u64 pml4)
{
    if (!image || size < sizeof(struct elf_ehdr))
        return 0;
    const struct elf_ehdr *eh = (const struct elf_ehdr *)image;
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' ||
        eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F')
        return 0;
    if (eh->e_ident[4] != 2) { kputs("[elf] not 64-bit\n"); return 0; }
    if (eh->e_machine != 62) { kputs("[elf] not x86-64\n"); return 0; }
    if (eh->e_type != 2 && eh->e_type != 3) { kputs("[elf] not EXEC/DYN\n"); return 0; }

    u16 phnum = eh->e_phnum;
    u16 phentsize = eh->e_phentsize;
    u64 phoff = eh->e_phoff;
    if (phentsize < sizeof(struct elf_phdr))
        return 0;

    u32 i;
    u64 max_end = 0;
    for (i = 0; i < phnum; i++) {
        const struct elf_phdr *ph = (const struct elf_phdr *)(image + phoff + i * phentsize);
        if (ph->p_type != PT_LOAD)
            continue;
        u64 vaddr = ph->p_vaddr;
        u64 memsz = ph->p_memsz;
        u64 filesz = ph->p_filesz;
        if (ph->p_offset + filesz > size)
            return 0;

        u64 page_start = vaddr & ~0xFFFULL;
        u64 page_end = (vaddr + memsz + 0xFFF) & ~0xFFFULL;
        u64 pa;
        for (pa = page_start; pa < page_end; pa += 0x1000) {
            u64 frame = pmm_alloc();
            if (!frame) return 0;
            u64 flags = PTE_U;
            if (ph->p_flags & PF_W) flags |= PTE_W;
            if (vmm_map_page(pml4, pa, frame, flags) < 0) {
                pmm_free(frame);
                return 0;
            }
            /* copy file data, then zero the remainder of the page */
            u64 off_in_page = 0;
            if (pa < vaddr)
                off_in_page = vaddr - pa;   /* page starts before the segment */
            u64 seg_off = pa + off_in_page - vaddr;   /* offset into the file */
            if (seg_off < filesz) {
                u64 n = filesz - seg_off;
                if (n > 0x1000 - off_in_page)
                    n = 0x1000 - off_in_page;
                memcpy((void *)frame, (void *)(u64)(image + ph->p_offset + seg_off), n);
                /* zero the rest of the page up to memsz */
                u64 zero_start = off_in_page + n;
                if (zero_start < 0x1000)
                    memset((void *)(frame + zero_start), 0, 0x1000 - zero_start);
            } else {
                memset((void *)frame, 0, 0x1000);
            }
        }
        if (vaddr + memsz > max_end)
            max_end = vaddr + memsz;
    }

    /* set the program break just after the loaded segments */
    extern u64 program_break;
    program_break = (max_end + 0xFFF) & ~0xFFFULL;

    kprintf("[elf] loaded, entry=0x%llx break=0x%llx\n", eh->e_entry, program_break);
    return eh->e_entry;
}

u64 elf_setup_stack(u64 user_stack_top, int argc, char **argv)
{
    /* Build a Linux-style initial stack:
     *   argc, argv[0..argc], NULL, envp[0..], NULL
     * Returns the stack pointer. */
    u64 *sp = (u64 *)user_stack_top;
    /* envp: none */
    *--sp = 0;                 /* NULL envp */
    /* argv (strings copied below) */
    /* count strings */
    int i;
    /* We'll lay out: [argc][argv pointers][NULL][envp NULL], with strings below. */
    /* Simpler: argc=1, argv[0]="slopos", argv[1]=NULL, envp=NULL */
    (void)argc; (void)argv;
    const char *name = "slopos";
    int nlen = (int)strlen(name) + 1;
    char *str = (char *)(sp) - nlen;
    str = (char *)((u64)str & ~0xFULL);
    memcpy((void *)str, name, nlen);
    sp = (u64 *)str;
    *--sp = 0;                 /* NULL envp */
    *--sp = 0;                 /* NULL argv terminator */
    *--sp = (u64)str;          /* argv[0] */
    *--sp = 1;                 /* argc */
    /* align the initial stack to 8 mod 16 (as gcc's _start expects) */
    if (((u64)sp & 0xF) == 0)
        sp = (u64 *)((u64)sp - 8);
    return (u64)sp;
}
