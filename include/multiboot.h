#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "types.h"

#define MULTIBOOT_MAGIC 0x2BADB002

typedef struct {
    uint32 flags;
    uint32 mem_lower;
    uint32 mem_upper;
    uint32 boot_device;
    uint32 cmdline;
    uint32 mods_count;
    uint32 mods_addr;
    uint32 syms[4];
    uint32 mmap_length;
    uint32 mmap_addr;
    uint32 drives_length;
    uint32 drives_addr;
    uint32 config_table;
    uint32 boot_loader_name;
    uint32 apm_table;
    uint32 vbe_control_info;
    uint32 vbe_mode_info;
    uint16 vbe_mode;
    uint16 vbe_interface_seg;
    uint16 vbe_interface_off;
    uint16 vbe_interface_len;
} __attribute__((packed)) multiboot_info_t;

typedef struct {
    uint32 size;
    uint64 base_addr;
    uint64 length;
    uint32 type;
} __attribute__((packed)) multiboot_mmap_entry_t;

#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED  2

void multiboot_parse_memory_map(multiboot_info_t *mbi);

#endif
