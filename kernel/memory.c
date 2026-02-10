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

static uint8_t memory_bitmap[BITMAP_SIZE];
static uint32_t page_directory[PAGE_DIRECTORY_SIZE] __attribute__((aligned(4096)));
static uint32_t first_page_table[PAGE_TABLE_SIZE] __attribute__((aligned(4096)));

static uint32_t memory_end = 0;
static uint8_t memory_initialized = 0;
static uint8_t paging_initialized = 0;

static void *heap_current = NULL;
static void *heap_end = NULL;


void multiboot_parse_memory_map(multiboot_info_t *mbi) {
    if (!(mbi->flags & (1 << 6))) {
        print("Memory map not available from bootloader\n");
        return;
    }

    print("\n=== Memory Map ===\n");
    
    multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)mbi->mmap_addr;
    multiboot_mmap_entry_t *mmap_end = (multiboot_mmap_entry_t *)(mbi->mmap_addr + mbi->mmap_length);
    
    while (mmap < mmap_end) {
        print("Base: 0x");
        print_hex((uint32_t)(mmap->base_addr >> 32));
        print_hex((uint32_t)(mmap->base_addr & 0xFFFFFFFF));
        print(" Length: 0x");
        print_hex((uint32_t)(mmap->length >> 32));
        print_hex((uint32_t)(mmap->length & 0xFFFFFFFF));
        print(" Type: ");
        
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            print("Available\n");
            uint64 end = mmap->base_addr + mmap->length;
            if (end > memory_end && end < 0x100000000ULL) {
                memory_end = (uint32_t)end;
            }
        } else {
            print("Reserved\n");
        }
        
        mmap = (multiboot_mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
    
    print("Detected memory end: 0x");
    print_hex(memory_end);
    print("\n");
    print("Kernel start: 0x");
    print_hex((uint32_t)&__kernel_section_start);
    print("\n");
    print("Kernel end: 0x");
    print_hex((uint32_t)&__kernel_section_end);
    print("\n\n");
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
    print("Initializing paging...\n");
    
    print("Page directory at: 0x");
    print_hex((uint32_t)page_directory);
    print("\n");
    print("Page table at: 0x");
    print_hex((uint32_t)first_page_table);
    print("\n");
    
    memset(page_directory, 0, sizeof(page_directory));
    memset(first_page_table, 0, sizeof(first_page_table));
    
    uint32_t kernel_end_addr = (uint32_t)&__kernel_section_end;
    uint32_t pages_to_map = (kernel_end_addr + PAGE_SIZE - 1) / PAGE_SIZE + 64;
    
    if (pages_to_map > PAGE_TABLE_SIZE) {
        pages_to_map = PAGE_TABLE_SIZE;
    }
    
    print("Mapping ");
    print_hex(pages_to_map);
    print(" pages\n");
    
    for (uint32_t i = 0; i < pages_to_map; i++) {
        uint32_t phys_addr = i * PAGE_SIZE;
        first_page_table[i] = phys_addr | PAGE_PRESENT | PAGE_WRITE;
    }
    
    page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_WRITE;
    
    paging_initialized = 1;
    print("Paging structures initialized\n");
}

void paging_enable() {
    if (!paging_initialized) {
        kernel_panic("Attempted to enable paging before initialization");
    }
    
    print("Enabling paging...\n");
    
    asm volatile(
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : 
        : "r" (page_directory)
        : "eax"
    );
    
    print("Paging enabled successfully\n");
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
        
        print("Heap initialized at 0x");
        print_hex((uint32_t)heap_current);
        print("\n");
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
    
    print("Memory initialized\n");
    print("Kernel pages marked as used: ");
    print_hex(kernel_start_page);
    print(" - ");
    print_hex(kernel_end_page);
    print("\n");
    
    memory_initialized = 1;
}
