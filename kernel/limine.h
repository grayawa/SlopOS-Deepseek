/* SlopOS Limine Protocol Header
 * Based on Limine boot protocol specification
 * SPDX-License-Identifier: 0BSD
 */
#ifndef LIMINE_H
#define LIMINE_H

#include <stdint.h>

#define LIMINE_COMMON_MAGIC 0xc7b1dd30df4c8b88, 0x0a82e883a194f07b

struct limine_uuid {
    uint32_t a;
    uint16_t b;
    uint16_t c;
    uint8_t d[8];
};

struct limine_file {
    uint64_t revision;
    void *address;
    uint64_t size;
    char *path;
    char *cmdline;
    uint32_t media_type;
    uint32_t unused;
    uint32_t tftp_ip;
    uint32_t tftp_port;
    uint32_t partition_index;
    uint32_t mbr_disk_id;
    struct limine_uuid gpt_disk_uuid;
    struct limine_uuid gpt_part_uuid;
    struct limine_uuid part_uuid;
};

/* Base revision numbers for requests/responses */
#define LIMINE_BOOTLOADER_INFO_REQUEST  { LIMINE_COMMON_MAGIC, 0xf55038d8e2a1202f, 0x279426fcf5f59740 }
#define LIMINE_EXECUTABLE_CMDLINE_REQUEST  { LIMINE_COMMON_MAGIC, 0x4b161536e596581d, 0x3906459697aab203 }
#define LIMINE_ENTRY_POINT_REQUEST   { LIMINE_COMMON_MAGIC, 0x13d86c0353bfb3a0, 0xe76211071df18183 }
#define LIMINE_FRAMEBUFFER_REQUEST   { LIMINE_COMMON_MAGIC, 0x9d5827dcd881dd75, 0xa3148604f6fab11b }
#define LIMINE_PAGING_MODE_REQUEST   { LIMINE_COMMON_MAGIC, 0x95c1a0edab0944cb, 0xa4e5ce384f54e040 }
#define LIMINE_5_LEVEL_PAGING_REQUEST { LIMINE_COMMON_MAGIC, 0x94469551da9b3192, 0xebe5e86db7382888 }
#define LIMINE_SMP_REQUEST            { LIMINE_COMMON_MAGIC, 0x95a67b819a1b857e, 0xa0b61b723b6a73e0 }
#define LIMINE_MEMORY_MAP_REQUEST     { LIMINE_COMMON_MAGIC, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62 }
#define LIMINE_HHDM_REQUEST           { LIMINE_COMMON_MAGIC, 0x48dcf1cb8ad2b852, 0x63984e959a98244b }
#define LIMINE_KERNEL_ADDRESS_REQUEST { LIMINE_COMMON_MAGIC, 0x71ba76863cc55f63, 0xb2644a48c516a487 }
#define LIMINE_KERNEL_FILE_REQUEST    { LIMINE_COMMON_MAGIC, 0xad97e90e4a18176a, 0x7a53d8a5e0a9f42d }
#define LIMINE_MODULE_REQUEST         { LIMINE_COMMON_MAGIC, 0x3e7e279702be32af, 0xca1c4f3bd1280cee }
#define LIMINE_RSDP_REQUEST           { LIMINE_COMMON_MAGIC, 0xc5e77b6b397e7b43, 0x27637845accdcf3c }
#define LIMINE_EFI_SYSTEM_TABLE_REQUEST { LIMINE_COMMON_MAGIC, 0xe8b39f1df817d08b, 0xcb86305a70de83fb }
#define LIMINE_EFI_MEMORY_MAP_REQUEST   { LIMINE_COMMON_MAGIC, 0x7df62a431d6872d5, 0xa4fcdfb3e57306c8 }
#define LIMINE_DTB_REQUEST              { LIMINE_COMMON_MAGIC, 0xb40ddb48fb54bac7, 0x545081493f81ffb7 }
#define LIMINE_SMBIOS_REQUEST           { LIMINE_COMMON_MAGIC, 0x9e9046f11e095391, 0xaa4a520fefbde5ee }
#define LIMINE_EFI_RUNTIME_SERVICES_REQUEST { LIMINE_COMMON_MAGIC, 0xe23ca029473cb663, 0x66020d6f0e2641cf }
#define LIMINE_TERMINAL_REQUEST           { LIMINE_COMMON_MAGIC, 0xc8ac59310c2b0844, 0xa68d0c7265d38878 }
#define LIMINE_MP_REQUEST                 { LIMINE_COMMON_MAGIC, 0x95a67b819a1b857e, 0xa0b61b723b6a73e0 }
#define LIMINE_X86_64_KEEP_IOMMU_REQUEST  { LIMINE_COMMON_MAGIC, 0x7a5a5e90f9cb3e14, 0xa53eaf9c84b13d07 }
#define LIMINE_BOOT_TIME_REQUEST          { LIMINE_COMMON_MAGIC, 0x502746e184c088aa, 0x6ceb6705391b3d0c }

struct limine_bootloader_info_response {
    uint64_t revision;
    char *name;
    char *version;
};

struct limine_executable_cmdline_response {
    uint64_t revision;
    char *cmdline;
};

struct limine_entry_point_response {
    uint64_t revision;
};

struct limine_framebuffer_response {
    uint64_t revision;
    uint64_t framebuffer_count;
    struct limine_framebuffer **framebuffers;
};

struct limine_framebuffer {
    void *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
    uint8_t unused[7];
    uint64_t edid_size;
    void *edid;
    uint64_t mode_count;
    struct limine_video_mode **modes;
};

struct limine_video_mode {
    uint64_t pitch;
    uint64_t width;
    uint64_t height;
    uint16_t bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};

#define LIMINE_PAGING_MODE_X86_64_4LVL 1
#define LIMINE_PAGING_MODE_X86_64_5LVL 4
#define LIMINE_PAGING_MODE_ARM64_4LVL  2
#define LIMINE_PAGING_MODE_ARM64_5LVL  3

struct limine_paging_mode_response {
    uint64_t revision;
    uint64_t mode;
    uint64_t flags;
};

struct limine_memmap_response {
    uint64_t revision;
    uint64_t entry_count;
    struct limine_memmap_entry **entries;
};

#define LIMINE_MEMMAP_USABLE                 0
#define LIMINE_MEMMAP_RESERVED               1
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2
#define LIMINE_MEMMAP_ACPI_NVS               3
#define LIMINE_MEMMAP_BAD_MEMORY             4
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define LIMINE_MEMMAP_EXECUTABLE_AND_MODULES 6
#define LIMINE_MEMMAP_FRAMEBUFFER            7

struct limine_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};

struct limine_hhdm_response {
    uint64_t revision;
    uint64_t offset;
};

struct limine_kernel_address_response {
    uint64_t revision;
    uint64_t physical_base;
    uint64_t virtual_base;
};

struct limine_kernel_file_response {
    uint64_t revision;
    struct limine_file *kernel_file;
};

struct limine_module_response {
    uint64_t revision;
    uint64_t module_count;
    struct limine_file **modules;
};

struct limine_rsdp_response {
    uint64_t revision;
    void *address;
};

struct limine_smbios_response {
    uint64_t revision;
    void *entry_32;
    void *entry_64;
};

struct limine_efi_system_table_response {
    uint64_t revision;
    void *address;
};

struct limine_efi_memory_map_response {
    uint64_t revision;
    void *memory_map;
    uint64_t memory_map_size;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
};

struct limine_mp_response {
    uint64_t revision;
    uint32_t flags;
    uint32_t bsp_lapic_id;
    uint64_t cpu_count;
    struct limine_mp_info **cpus;
};

struct limine_mp_info {
    uint32_t processor_id;
    uint32_t lapic_id;
    uint64_t reserved;
    void (*goto_address)(struct limine_mp_info *);
    uint64_t extra_argument;
};

struct limine_dtb_response {
    uint64_t revision;
    void *dtb_ptr;
};

struct limine_terminal_response {
    uint64_t revision;
    uint64_t terminal_count;
    struct limine_terminal **terminals;
    void (*write)(struct limine_terminal *, const char *, uint64_t);
};

struct limine_terminal {
    uint64_t columns;
    uint64_t rows;
    struct limine_framebuffer *framebuffer;
};

struct limine_boot_time_response {
    uint64_t revision;
    int64_t boot_time;
};

struct limine_x86_64_keep_iommu_response {
    uint64_t revision;
};

/* Request structs */
struct limine_bootloader_info_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_bootloader_info_response *response;
};

struct limine_executable_cmdline_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_executable_cmdline_response *response;
};

struct limine_entry_point_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_entry_point_response *response;
    void *entry;
};

struct limine_framebuffer_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_framebuffer_response *response;
};

struct limine_paging_mode_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_paging_mode_response *response;
    uint64_t mode;
    uint64_t max_mode;
    uint64_t min_mode;
};

struct limine_memmap_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_memmap_response *response;
};

struct limine_hhdm_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_hhdm_response *response;
};

struct limine_kernel_address_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_kernel_address_response *response;
};

struct limine_kernel_file_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_kernel_file_response *response;
};

struct limine_module_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_module_response *response;
};

struct limine_rsdp_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_rsdp_response *response;
};

struct limine_smbios_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_smbios_response *response;
};

struct limine_efi_system_table_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_efi_system_table_response *response;
};

struct limine_terminal_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_terminal_response *response;
    void (*callback)(struct limine_terminal *, uint64_t, uint64_t, uint64_t);
};

struct limine_mp_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_mp_response *response;
    uint64_t flags;
};

struct limine_boot_time_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_boot_time_response *response;
};

struct limine_x86_64_keep_iommu_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_x86_64_keep_iommu_response *response;
};

#endif
