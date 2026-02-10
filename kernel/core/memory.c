#include "memory.h"
#include "types.h"
#include "string.h"
#include "video.h"
#include "io_ports.h"
#include "kernel.h"
#include "multiboot.h"


#define PAGE_SIZE 4096
#define MEMORY_SIZE 0x1000000
#define BITMAP_SIZE (MEMORY_SIZE / PAGE_SIZE / 8)
#define TOTAL_PAGES (MEMORY_SIZE / PAGE_SIZE)

#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024

#define PAGE_PRESENT    0x1
#define PAGE_WRITE      0x2
#define PAGE_USER       0x4

// Page directory structure
struct page_directory {
    uint32_t entries[PAGE_DIRECTORY_SIZE];
    uint32_t* tables[PAGE_DIRECTORY_SIZE];  // Physical addresses of page tables
} __attribute__((aligned(4096)));

typedef struct page_directory page_directory_t;

static uint8_t memory_bitmap[BITMAP_SIZE];
static page_directory_t kernel_page_directory __attribute__((aligned(4096)));
uint32_t first_page_table[PAGE_TABLE_SIZE] __attribute__((aligned(4096)));

static uint32_t memory_end = 0;
static uint8_t memory_initialized = 0;
static uint8_t paging_initialized = 0;

static void *heap_current = NULL;
static void *heap_end = NULL;


void multiboot_parse_memory_map(multiboot_info_t *mbi) {
    if (!(mbi->flags & (1 << 6))) {
        debug_print("Memory map not available from bootloader\n");
        return;
    }

    debug_print("\n=== Memory Map ===\n");
    
    multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)mbi->mmap_addr;
    multiboot_mmap_entry_t *mmap_end = (multiboot_mmap_entry_t *)(mbi->mmap_addr + mbi->mmap_length);
    
    while (mmap < mmap_end) {
        debug_print("Base: 0x");
        debug_print_hex((uint32_t)(mmap->base_addr >> 32));
        debug_print_hex((uint32_t)(mmap->base_addr & 0xFFFFFFFF));
        debug_print(" Length: 0x");
        debug_print_hex((uint32_t)(mmap->length >> 32));
        debug_print_hex((uint32_t)(mmap->length & 0xFFFFFFFF));
        debug_print(" Type: ");
        
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            debug_print("Available\n");
            uint64 end = mmap->base_addr + mmap->length;
            if (end > memory_end && end < 0x100000000ULL) {
                memory_end = (uint32_t)end;
            }
        } else {
            debug_print("Reserved\n");
        }
        
        mmap = (multiboot_mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
    
    debug_print("Detected memory end: 0x");
    debug_print_hex(memory_end);
    debug_print("\n");
    debug_print("Kernel start: 0x");
    debug_print_hex((uint32_t)&__kernel_section_start);
    debug_print("\n");
    debug_print("Kernel end: 0x");
    debug_print_hex((uint32_t)&__kernel_section_end);
    debug_print("\n\n");
}

int is_page_used(size_t page) {
    return memory_bitmap[page / 8] & (1 << (page % 8));
}

int no_free_pages() {
    for (int i = 0; i < TOTAL_PAGES; i++) {
        if (!is_page_used(i)) {
            return 0;
        }
    }
    return 1;
}

void set_page_used(size_t page) {
    memory_bitmap[page / 8] |= (1 << (page % 8));
}

void set_page_free(size_t page) {
    memory_bitmap[page / 8] &= ~(1 << (page % 8));
}

void *get_free_page() {
    for (int i = 1; i < TOTAL_PAGES; i++) {
        if (!is_page_used(i)) {
            set_page_used(i);
            return (void *)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void *allocate_page() {
    if (no_free_pages()) {
        kernel_panic("No free pages available - OOM");
    }

    void *page = get_free_page();
    if (page == NULL) {
        kernel_panic("Failed to get a free page - OOM");
    }

    return page;
}

void free_page(void *ptr) {
    size_t page = (size_t)ptr / PAGE_SIZE;
    set_page_free(page);
}

void paging_init() {
    debug_print("Initializing paging...\n");
    
    debug_print("Page directory at: 0x");
    debug_print_hex((uint32_t)&kernel_page_directory);
    debug_print("\n");
    debug_print("Page table at: 0x");
    debug_print_hex((uint32_t)first_page_table);
    debug_print("\n");
    
    memset(&kernel_page_directory, 0, sizeof(kernel_page_directory));
    memset(first_page_table, 0, sizeof(first_page_table));
    
    // Map all available usable memory with identity paging
    // This ensures that virtual addresses = physical addresses for kernel code and heap
    // Use memory_end that was detected from multiboot memory map
    uint32_t pages_to_map = memory_end / PAGE_SIZE;
    
    if (pages_to_map > PAGE_TABLE_SIZE) {
        pages_to_map = PAGE_TABLE_SIZE;
    }
    
    debug_print("Mapping ");
    debug_print_hex(pages_to_map);
    debug_print(" pages\n");
    
    for (uint32_t i = 0; i < pages_to_map; i++) {
        uint32_t phys_addr = i * PAGE_SIZE;
        first_page_table[i] = phys_addr | PAGE_PRESENT | PAGE_WRITE;
    }
    
    kernel_page_directory.entries[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    kernel_page_directory.tables[0] = first_page_table;
    
    paging_initialized = 1;
    debug_print("Paging structures initialized\n");
}

void paging_enable() {
    if (!paging_initialized) {
        kernel_panic("Attempted to enable paging before initialization");
    }
    
    debug_print("Enabling paging...\n");
    
    asm volatile(
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : 
        : "r" (&kernel_page_directory)
        : "eax"
    );
    
    debug_print("Paging enabled successfully\n");
}

// Get kernel page directory
page_directory_t* get_kernel_page_directory(void) {
    return &kernel_page_directory;
}

// Create a new page directory for a process
page_directory_t* create_page_directory(void) {
    // Allocate page directory structure
    page_directory_t* dir = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!dir) {
        return NULL;
    }
    
    memset(dir, 0, sizeof(page_directory_t));
    
    // Copy kernel mappings from the kernel page directory
    // This ensures kernel code is accessible from all processes
    // Important: Only copy the entries (which contain physical addresses),
    // NOT the virtual pointers in tables[]
    for (int dir_index = 0; dir_index < PAGE_DIRECTORY_SIZE; dir_index++) {
        if (kernel_page_directory.entries[dir_index] != 0) {
            // Copy kernel page directory entries (these contain physical addresses)
            dir->entries[dir_index] = kernel_page_directory.entries[dir_index];
            // Extract the physical address from the entry and get its virtual pointer
            uint32_t phys_addr = kernel_page_directory.entries[dir_index] & 0xFFFFF000;
            dir->tables[dir_index] = (uint32_t*)phys_addr;
        }
    }
    
    return dir;
}

// Switch to a different page directory
void switch_page_directory(page_directory_t* dir) {
    if (!dir) {
        kernel_panic("Attempted to switch to NULL page directory");
    }
    
    // Get physical address of the page directory
    // With identity paging (virt=phys), cast pointer to physical address
    uint32_t phys_addr = (uint32_t)dir;
    
    // Load the physical address into CR3
    asm volatile(
        "mov %0, %%cr3\n"
        :
        : "r" (phys_addr)
        : "memory"
    );
}

// Map a page in a specific directory
void map_page_in_directory(page_directory_t* dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    if (!dir) {
        kernel_panic("Attempted to map page in NULL directory");
    }
    
    uint32_t dir_index = virt_addr / (PAGE_SIZE * PAGE_TABLE_SIZE);
    uint32_t table_index = (virt_addr / PAGE_SIZE) % PAGE_TABLE_SIZE;
    
    // Check if page table exists
    // Invariant: dir->tables[dir_index] and dir->entries[dir_index] should be consistent
    // Either both NULL/zero or both set
    if (dir->tables[dir_index] == NULL) {
        // Verify consistency - entry should also be empty
        if (dir->entries[dir_index] != 0) {
            kernel_panic("Page directory inconsistent: entry set but table NULL");
        }
        
        // Allocate a new page table
        uint32_t* new_table = (uint32_t*)kmalloc(PAGE_TABLE_SIZE * sizeof(uint32_t));
        if (!new_table) {
            kernel_panic("Failed to allocate page table");
        }
        memset(new_table, 0, PAGE_TABLE_SIZE * sizeof(uint32_t));
        
        // Update directory entry
        dir->entries[dir_index] = ((uint32_t)new_table) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        dir->tables[dir_index] = new_table;
    }
    
    // Map the page in the table
    dir->tables[dir_index][table_index] = phys_addr | flags;
}

void *kmalloc(size_t size) {
    if (!memory_initialized) {
        kernel_panic("kmalloc called before memory initialization");
    }
    
    if (size == 0) {
        kernel_panic("kmalloc called with size 0");
    }
    
    size_t aligned_size = (size + 15) & ~15;
    
    if (heap_current == NULL) {
        uint32_t kernel_end = (uint32_t)&__kernel_section_end;
        heap_current = (void *)((kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
        heap_end = (void *)(memory_end > MEMORY_SIZE ? MEMORY_SIZE : memory_end);
        
        if (heap_current >= heap_end) {
            kernel_panic("Heap initialization failed - no space after kernel");
        }
        
        debug_print("Heap initialized at 0x");
        debug_print_hex((uint32_t)heap_current);
        debug_print("\n");
    }
    
    void *result = heap_current;
    heap_current = (void *)((uint32_t)heap_current + aligned_size);
    
    if (heap_current > heap_end) {
        kernel_panic("kmalloc OOM - heap exhausted");
    }
    
    return result;
}

void kfree(void *ptr) {
    (void)ptr;
}

void memory_init(multiboot_info_t *mbi) {
    memset(memory_bitmap, 0, BITMAP_SIZE);

    for (int i = 0; i < TOTAL_PAGES; i++) {
        set_page_free(i);
    }
    
    multiboot_parse_memory_map(mbi);
    
    uint32_t kernel_start = (uint32_t)&__kernel_section_start;
    uint32_t kernel_end = (uint32_t)&__kernel_section_end;
    
    uint32_t kernel_start_page = kernel_start / PAGE_SIZE;
    uint32_t kernel_end_page = (kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    
    for (uint32_t i = 0; i <= kernel_end_page; i++) {
        set_page_used(i);
    }
    
    debug_print("Memory initialized\n");
    debug_print("Kernel pages marked as used: ");
    debug_print_hex(kernel_start_page);
    debug_print(" - ");
    debug_print_hex(kernel_end_page);
    debug_print("\n");
    
    memory_initialized = 1;
}
