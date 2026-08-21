#include "vmm.h"
#include "pmm.h"
#include "lib.h"
#include "port.h"
#include "printk.h"

/* boot page tables set up in boot.S (identity-maps low 4 GiB) */
extern u64 pml4[512];

static u64 kernel_pml4;

#define PDPT_INDEX(v) (((v) >> 30) & 511)
#define PD_INDEX(v)   (((v) >> 21) & 511)
#define PT_INDEX(v)   (((v) >> 12) & 511)
#define PML4_INDEX(v) (((v) >> 39) & 511)

static inline u64 *table_at(u64 phys)
{
    /* kernel is identity mapped, so phys == virt for < 4 GiB */
    return (u64 *)(u64)phys;
}

/* allocate and zero a page-table frame, returning its physical address */
static u64 alloc_table(void)
{
    u64 p = pmm_alloc();
    if (p)
        memset((void *)p, 0, 4096);
    return p;
}

void vmm_init(void)
{
    kernel_pml4 = (u64)&pml4[0];
    vmm_switch(kernel_pml4);
    kprintf("[vmm] kernel pml4 at 0x%llx\n", kernel_pml4);
}

u64 vmm_kernel_pml4(void)
{
    return kernel_pml4;
}

void vmm_switch(u64 pml4)
{
    write_cr3(pml4);
}

u64 vmm_create_pml4(void)
{
    u64 new = alloc_table();
    if (!new) return 0;
    /* clone the kernel mappings so kernel code/data stay reachable */
    memcpy((void *)new, (void *)kernel_pml4, 4096);
    return new;
}

int vmm_map_page(u64 pml4_addr, u64 virt, u64 phys, u64 flags)
{
    u64 *pml4 = table_at(pml4_addr);
    u64 i4 = PML4_INDEX(virt);
    u64 i3 = PDPT_INDEX(virt);
    u64 i2 = PD_INDEX(virt);
    u64 i1 = PT_INDEX(virt);
    u64 up = flags & PTE_U;   /* propagate the user bit up the hierarchy */

    if (!(pml4[i4] & PTE_P)) {
        u64 t = alloc_table();
        if (!t) return -1;
        pml4[i4] = t | PTE_P | PTE_W | up;
    }
    u64 *pdpt = table_at(pml4[i4] & PTE_ADDR_MASK);
    if (!(pdpt[i3] & PTE_P)) {
        u64 t = alloc_table();
        if (!t) return -1;
        pdpt[i3] = t | PTE_P | PTE_W | up;
    }
    u64 *pd = table_at(pdpt[i3] & PTE_ADDR_MASK);
    if (!(pd[i2] & PTE_P)) {
        u64 t = alloc_table();
        if (!t) return -1;
        pd[i2] = t | PTE_P | PTE_W | up;
    } else if (pd[i2] & PTE_PS) {
        /* huge page present; split into 4K pages */
        u64 t = alloc_table();
        if (!t) return -1;
        u64 base = pd[i2] & PTE_ADDR_MASK;
        u64 n;
        for (n = 0; n < 512; n++)
            table_at(t)[n] = (base + (n << 12)) | (pd[i2] & 0xFFF);
        pd[i2] = t | PTE_P | PTE_W | up;
    }
    /* ensure the user bit is set at every level (the kernel clone may be supervisor) */
    pml4[i4] |= up;
    pdpt[i3] |= up;
    pd[i2] |= up;
    u64 *pt = table_at(pd[i2] & PTE_ADDR_MASK);
    pt[i1] = (phys & PTE_ADDR_MASK) | flags | PTE_P;
    invlpg(virt);
    return 0;
}

int vmm_map_2mb(u64 pml4_addr, u64 virt, u64 phys, u64 flags)
{
    u64 *pml4 = table_at(pml4_addr);
    u64 i4 = PML4_INDEX(virt);
    u64 i3 = PDPT_INDEX(virt);
    u64 i2 = PD_INDEX(virt);
    u64 up = flags & PTE_U;
    if (!(pml4[i4] & PTE_P)) {
        u64 t = alloc_table();
        if (!t) return -1;
        pml4[i4] = t | PTE_P | PTE_W | up;
    }
    u64 *pdpt = table_at(pml4[i4] & PTE_ADDR_MASK);
    if (!(pdpt[i3] & PTE_P)) {
        u64 t = alloc_table();
        if (!t) return -1;
        pdpt[i3] = t | PTE_P | PTE_W | up;
    }
    u64 *pd = table_at(pdpt[i3] & PTE_ADDR_MASK);
    pd[i2] = (phys & PTE_ADDR_MASK) | flags | PTE_P | PTE_PS;
    invlpg(virt);
    return 0;
}

int vmm_alloc_page(u64 pml4, u64 virt, u64 flags)
{
    u64 phys = pmm_alloc();
    if (!phys) return -1;
    return vmm_map_page(pml4, virt, phys, flags);
}

int vmm_map_pages(u64 pml4, u64 virt, u64 phys, u64 n, u64 flags)
{
    u64 i;
    for (i = 0; i < n; i++) {
        if (vmm_map_page(pml4, virt + i * 4096, phys + i * 4096, flags) < 0)
            return -1;
    }
    return 0;
}

int vmm_alloc_pages(u64 pml4, u64 virt, u64 n, u64 flags)
{
    u64 i;
    for (i = 0; i < n; i++) {
        if (vmm_alloc_page(pml4, virt + i * 4096, flags) < 0)
            return -1;
    }
    return 0;
}

int vmm_unmap_page(u64 pml4_addr, u64 virt)
{
    u64 *pml4 = table_at(pml4_addr);
    u64 i4 = PML4_INDEX(virt);
    u64 i3 = PDPT_INDEX(virt);
    u64 i2 = PD_INDEX(virt);
    u64 i1 = PT_INDEX(virt);
    if (!(pml4[i4] & PTE_P)) return -1;
    u64 *pdpt = table_at(pml4[i4] & PTE_ADDR_MASK);
    if (!(pdpt[i3] & PTE_P)) return -1;
    u64 *pd = table_at(pdpt[i3] & PTE_ADDR_MASK);
    if (!(pd[i2] & PTE_P) || (pd[i2] & PTE_PS)) return -1;
    u64 *pt = table_at(pd[i2] & PTE_ADDR_MASK);
    pt[i1] = 0;
    invlpg(virt);
    return 0;
}

u64 vmm_virt_to_phys(u64 pml4_addr, u64 virt)
{
    u64 *pml4 = table_at(pml4_addr);
    u64 i4 = PML4_INDEX(virt);
    u64 i3 = PDPT_INDEX(virt);
    u64 i2 = PD_INDEX(virt);
    u64 i1 = PT_INDEX(virt);
    if (!(pml4[i4] & PTE_P)) return 0;
    u64 *pdpt = table_at(pml4[i4] & PTE_ADDR_MASK);
    if (!(pdpt[i3] & PTE_P)) return 0;
    u64 *pd = table_at(pdpt[i3] & PTE_ADDR_MASK);
    if (!(pd[i2] & PTE_P)) return 0;
    if (pd[i2] & PTE_PS)
        return (pd[i2] & PTE_ADDR_MASK) + (virt & 0x1FFFFF);
    u64 *pt = table_at(pd[i2] & PTE_ADDR_MASK);
    if (!(pt[i1] & PTE_P)) return 0;
    return (pt[i1] & PTE_ADDR_MASK) + (virt & 0xFFF);
}

void vmm_free_pml4(u64 pml4_addr)
{
    /* Free all user page tables and frames (not the kernel shared mappings). */
    u64 *pml4 = table_at(pml4_addr);
    int i;
    for (i = 0; i < 512; i++) {
        if (i == PML4_INDEX(0ULL)) continue;   /* skip kernel low mapping */
        if (pml4[i] & PTE_P) {
            u64 *pdpt = table_at(pml4[i] & PTE_ADDR_MASK);
            int j;
            for (j = 0; j < 512; j++) {
                if (pdpt[j] & PTE_P) {
                    u64 *pd = table_at(pdpt[j] & PTE_ADDR_MASK);
                    int k;
                    for (k = 0; k < 512; k++) {
                        if ((pd[k] & PTE_P) && !(pd[k] & PTE_PS)) {
                            u64 *pt = table_at(pd[k] & PTE_ADDR_MASK);
                            int l;
                            for (l = 0; l < 512; l++) {
                                if (pt[l] & PTE_P)
                                    pmm_free(pt[l] & PTE_ADDR_MASK);
                            }
                            pmm_free(pd[k] & PTE_ADDR_MASK);
                        }
                    }
                    pmm_free(pdpt[j] & PTE_ADDR_MASK);
                }
            }
            pmm_free(pml4[i] & PTE_ADDR_MASK);
        }
    }
    pmm_free(pml4_addr);
}
