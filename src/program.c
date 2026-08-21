#include "program.h"
#include "elf.h"
#include "task.h"
#include "vmm.h"
#include "lib.h"
#include "printk.h"

static u64 mod_addr, mod_size;

void program_register(const char *name, u64 addr, u64 size)
{
    (void)name;
    mod_addr = addr;
    mod_size = size;
}

int program_run(const char *name)
{
    if (mod_addr == 0 || mod_size == 0) {
        kputs("[program] no ELF module registered\n");
        return -1;
    }
    u64 user_stack_top;
    u64 pml4 = elf_create_pml4(&user_stack_top);
    if (!pml4) return -1;

    u64 entry = elf_load((const u8 *)mod_addr, mod_size, pml4);
    if (!entry) {
        kputs("[program] failed to load ELF\n");
        return -1;
    }
    u64 sp = elf_setup_stack(user_stack_top, 1, NULL);
    task_t *t = task_create_user(name, entry, sp, pml4);
    if (!t) return -1;

    kprintf("[program] running '%s'\n", name);
    sched_yield();   /* run the user task; returns when it exits */
    return 0;
}
