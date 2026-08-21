#include "program.h"
#include "elf.h"
#include "task.h"
#include "vmm.h"
#include "lib.h"
#include "printk.h"

#define MAX_MODULES 8

typedef struct {
    char name[32];
    u64 addr;
    u64 size;
} program_module_t;

static program_module_t modules[MAX_MODULES];
static int nmodules;

void program_register(const char *name, u64 addr, u64 size)
{
    if (nmodules >= MAX_MODULES)
        return;
    strncpy(modules[nmodules].name, name, 31);
    modules[nmodules].addr = addr;
    modules[nmodules].size = size;
    nmodules++;
    kprintf("[program] registered '%s' (0x%llx, %llu bytes)\n", name, addr, size);
}

int program_run(const char *name)
{
    int i;
    for (i = 0; i < nmodules; i++) {
        if (strcmp(modules[i].name, name) == 0)
            break;
    }
    if (i >= nmodules) {
        kputs("[program] unknown program: ");
        kputs(name);
        kputs("\n");
        return -1;
    }
    u64 addr = modules[i].addr;
    u64 size = modules[i].size;

    u64 user_stack_top;
    u64 pml4 = elf_create_pml4(&user_stack_top);
    if (!pml4) return -1;

    u64 entry = elf_load((const u8 *)addr, size, pml4);
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
