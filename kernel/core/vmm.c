#include "vmm.h"
#include "pmm.h"
#include "kheap.h"
#include "string.h"
#include "video.h"
#include "kernel.h"

/* Global VMM state */
static struct {
    page_directory_t* kernel_dir;   /* Kernel page directory */
    page_directory_t* current_dir;  /* Current page directory */
    vmm_stats_t stats;
    uint8_t initialized;
} g_vmm;

/* Kernel space starts at 3GB */
#define KERNEL_SPACE_START 0xC0000000

/* Get page table for a virtual address */
static uint32_t* get_page_table(page_directory_t* dir, uint32_t virt, int create) {
    uint32_t dir_index = VMM_DIR_INDEX(virt);
    
    /* Check if page table exists */
    if (dir->tables[dir_index]) {
        return dir->tables[dir_index];
    }
    
    if (!create) {
        return NULL;
    }
    
    /* Allocate new page table */
    uint32_t phys_frame = pmm_alloc_frame();
    if (phys_frame == 0) {
        return NULL;
    }
    
    uint32_t* table = (uint32_t*)PMM_FRAME_TO_ADDR(phys_frame);
    
    /* Clear the page table */
    memset(table, 0, VMM_PAGE_SIZE);
    
    /* Set directory entry */
    dir->entries[dir_index] = (uint32_t)table | VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE | VMM_FLAG_USER;
    dir->tables[dir_index] = table;
    
    return table;
}

int vmm_init(void) {
    if (g_vmm.initialized) {
        return 0;
    }
    
    /* Allocate kernel page directory */
    g_vmm.kernel_dir = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!g_vmm.kernel_dir) {
        return -1;
    }
    
    memset(g_vmm.kernel_dir, 0, sizeof(page_directory_t));
    g_vmm.kernel_dir->ref_count = 1;
    g_vmm.kernel_dir->vma_list = NULL;
    
    /* Identity map first 4MB (kernel code + data) */
    for (uint32_t addr = 0; addr < 0x400000; addr += VMM_PAGE_SIZE) {
        if (vmm_map_page(g_vmm.kernel_dir, addr, addr, VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE) != 0) {
            return -1;
        }
    }
    
    /* Map kernel heap area (identity mapped for now) */
    /* This would be expanded when heap grows */
    
    g_vmm.current_dir = g_vmm.kernel_dir;
    g_vmm.stats.total_pages = 1024; /* 4MB / 4KB */
    g_vmm.stats.kernel_pages = 1024;
    g_vmm.stats.user_pages = 0;
    g_vmm.stats.vma_count = 0;
    g_vmm.initialized = 1;
    
    debug_print("VMM: initialized with kernel directory at 0x");
    debug_print_hex((uint32_t)g_vmm.kernel_dir);
    debug_print("\n");
    
    return 0;
}

page_directory_t* vmm_create_directory(void) {
    /* Allocate directory structure */
    page_directory_t* dir = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!dir) {
        return NULL;
    }
    
    memset(dir, 0, sizeof(page_directory_t));
    dir->ref_count = 1;
    dir->vma_list = NULL;
    
    /* Copy kernel space mappings (above 3GB) */
    if (g_vmm.kernel_dir) {
        for (int i = 768; i < 1024; i++) {
            dir->entries[i] = g_vmm.kernel_dir->entries[i];
            dir->tables[i] = g_vmm.kernel_dir->tables[i];
        }
    }
    
    debug_print("VMM: created new directory at 0x");
    debug_print_hex((uint32_t)dir);
    debug_print("\n");
    
    return dir;
}

void vmm_destroy_directory(page_directory_t* dir) {
    if (!dir || dir == g_vmm.kernel_dir) {
        return; /* Don't destroy kernel directory */
    }
    
    /* Free user-space page tables */
    for (int i = 0; i < 768; i++) {
        if (dir->tables[i]) {
            /* Free the physical frame */
            uint32_t phys = (uint32_t)dir->tables[i];
            pmm_free_frame(PMM_ADDR_TO_FRAME(phys));
        }
    }
    
    /* Free VMAs */
    vma_t* vma = dir->vma_list;
    while (vma) {
        vma_t* next = vma->next;
        kfree(vma);
        vma = next;
    }
    
    kfree(dir);
}

void vmm_switch_directory(page_directory_t* dir) {
    if (!dir) {
        return;
    }
    
    g_vmm.current_dir = dir;
    
    /* Load CR3 with physical address of directory */
    uint32_t phys;
    if (vmm_get_physical(g_vmm.kernel_dir, (uint32_t)dir, &phys)) {
        asm volatile("mov %0, %%cr3" : : "r"(phys));
    }
}

page_directory_t* vmm_get_current_directory(void) {
    return g_vmm.current_dir;
}

int vmm_map_page(page_directory_t* dir, uint32_t virt, uint32_t phys, uint32_t flags) {
    if (!dir) {
        dir = g_vmm.current_dir;
    }
    
    if (!dir) {
        return -1;
    }
    
    /* Align addresses */
    virt &= VMM_PAGE_MASK;
    phys &= VMM_PAGE_MASK;
    
    /* Get or create page table */
    uint32_t* table = get_page_table(dir, virt, 1);
    if (!table) {
        return -1;
    }
    
    /* Set page table entry */
    uint32_t table_index = VMM_TABLE_INDEX(virt);
    table[table_index] = phys | (flags & 0xFFF) | VMM_FLAG_PRESENT;
    
    /* Update stats */
    g_vmm.stats.total_pages++;
    if (virt >= KERNEL_SPACE_START) {
        g_vmm.stats.kernel_pages++;
    } else {
        g_vmm.stats.user_pages++;
    }
    
    return 0;
}

void vmm_unmap_page(page_directory_t* dir, uint32_t virt) {
    if (!dir) {
        dir = g_vmm.current_dir;
    }
    
    if (!dir) {
        return;
    }
    
    virt &= VMM_PAGE_MASK;
    
    uint32_t* table = get_page_table(dir, virt, 0);
    if (!table) {
        return;
    }
    
    uint32_t table_index = VMM_TABLE_INDEX(virt);
    table[table_index] = 0;
    
    /* Invalidate TLB entry */
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    
    g_vmm.stats.total_pages--;
}

int vmm_get_physical(page_directory_t* dir, uint32_t virt, uint32_t* phys_out) {
    if (!dir) {
        dir = g_vmm.current_dir;
    }
    
    if (!dir || !phys_out) {
        return 0;
    }
    
    virt &= VMM_PAGE_MASK;
    
    uint32_t* table = get_page_table(dir, virt, 0);
    if (!table) {
        return 0;
    }
    
    uint32_t table_index = VMM_TABLE_INDEX(virt);
    uint32_t entry = table[table_index];
    
    if (!(entry & VMM_FLAG_PRESENT)) {
        return 0;
    }
    
    *phys_out = entry & VMM_PAGE_MASK;
    return 1;
}

int vmm_is_mapped(page_directory_t* dir, uint32_t virt) {
    uint32_t phys;
    return vmm_get_physical(dir, virt, &phys);
}

int vmm_alloc_page(page_directory_t* dir, uint32_t virt, uint32_t flags) {
    /* Allocate physical frame */
    uint32_t frame = pmm_alloc_frame();
    if (frame == 0) {
        return -1;
    }
    
    uint32_t phys = PMM_FRAME_TO_ADDR(frame);
    
    /* Map it */
    if (vmm_map_page(dir, virt, phys, flags) != 0) {
        pmm_free_frame(frame);
        return -1;
    }
    
    return 0;
}

int vmm_alloc_pages(page_directory_t* dir, uint32_t virt, uint32_t count, uint32_t flags) {
    for (uint32_t i = 0; i < count; i++) {
        if (vmm_alloc_page(dir, virt + i * VMM_PAGE_SIZE, flags) != 0) {
            /* Rollback */
            for (uint32_t j = 0; j < i; j++) {
                vmm_unmap_page(dir, virt + j * VMM_PAGE_SIZE);
            }
            return -1;
        }
    }
    return 0;
}

void vmm_free_pages(page_directory_t* dir, uint32_t virt, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint32_t addr = virt + i * VMM_PAGE_SIZE;
        
        /* Get physical address before unmapping */
        uint32_t phys;
        if (vmm_get_physical(dir, addr, &phys)) {
            pmm_free_frame(PMM_ADDR_TO_FRAME(phys));
        }
        
        vmm_unmap_page(dir, addr);
    }
}

vma_t* vmm_create_vma(page_directory_t* dir, uint32_t start, uint32_t size, 
                      uint32_t flags, vma_type_t type) {
    if (!dir) {
        return NULL;
    }
    
    /* Align start and size */
    start = (start + VMM_PAGE_SIZE - 1) & VMM_PAGE_MASK;
    size = (size + VMM_PAGE_SIZE - 1) & VMM_PAGE_MASK;
    
    /* Allocate VMA structure */
    vma_t* vma = (vma_t*)kmalloc(sizeof(vma_t));
    if (!vma) {
        return NULL;
    }
    
    vma->start = start;
    vma->end = start + size;
    vma->flags = flags;
    vma->type = type;
    vma->next = NULL;
    
    /* Add to directory's VMA list */
    if (!dir->vma_list) {
        dir->vma_list = vma;
    } else {
        vma_t* current = dir->vma_list;
        while (current->next) {
            current = current->next;
        }
        current->next = vma;
    }
    
    g_vmm.stats.vma_count++;
    
    return vma;
}

uint32_t vmm_find_free_region(page_directory_t* dir, uint32_t size, uint32_t hint) {
    if (!dir) {
        return 0;
    }
    
    /* Align size */
    size = (size + VMM_PAGE_SIZE - 1) & VMM_PAGE_MASK;
    
    /* Default user space range: 1MB to 3GB */
    uint32_t start = hint ? hint : 0x100000;
    uint32_t end = KERNEL_SPACE_START;
    
    /* Check if hint region is free */
    if (hint) {
        int free = 1;
        for (uint32_t addr = hint; addr < hint + size; addr += VMM_PAGE_SIZE) {
            if (vmm_is_mapped(dir, addr)) {
                free = 0;
                break;
            }
        }
        if (free) {
            return hint;
        }
    }
    
    /* Scan for free region */
    for (uint32_t addr = start; addr < end - size; addr += VMM_PAGE_SIZE) {
        int free = 1;
        for (uint32_t check = addr; check < addr + size; check += VMM_PAGE_SIZE) {
            if (vmm_is_mapped(dir, check)) {
                free = 0;
                addr = check; /* Skip to after this mapped page */
                break;
            }
        }
        if (free) {
            return addr;
        }
    }
    
    return 0; /* No free region found */
}

void vmm_get_stats(vmm_stats_t* stats) {
    if (stats) {
        *stats = g_vmm.stats;
        stats->initialized = g_vmm.initialized;
    }
}

void vmm_dump(void) {
    debug_print("\n=== Virtual Memory Manager ===\n");
    debug_print("Kernel directory: 0x");
    debug_print_hex((uint32_t)g_vmm.kernel_dir);
    debug_print("\nCurrent directory: 0x");
    debug_print_hex((uint32_t)g_vmm.current_dir);
    debug_print("\nTotal pages: ");
    debug_print_hex(g_vmm.stats.total_pages);
    debug_print("\nUser pages: ");
    debug_print_hex(g_vmm.stats.user_pages);
    debug_print("\nKernel pages: ");
    debug_print_hex(g_vmm.stats.kernel_pages);
    debug_print("\nVMA count: ");
    debug_print_hex(g_vmm.stats.vma_count);
    debug_print("\n");
    
    if (g_vmm.current_dir && g_vmm.current_dir->vma_list) {
        debug_print("VMAs:\n");
        vma_t* vma = g_vmm.current_dir->vma_list;
        while (vma) {
            debug_print("  0x");
            debug_print_hex(vma->start);
            debug_print(" - 0x");
            debug_print_hex(vma->end);
            debug_print(" type ");
            debug_print_hex(vma->type);
            debug_print("\n");
            vma = vma->next;
        }
    }
    debug_print("===============================\n");
}