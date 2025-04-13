#include "memory.h"
#include "types.h"
#include "string.h"
#include "video.h"
#include "io_ports.h"


#define PAGE_SIZE 4096
#define MEMORY_SIZE 0x1000000
#define BITMAP_SIZE (MEMORY_SIZE / PAGE_SIZE / 8)

static uint8_t memory_bitmap[BITMAP_SIZE];

void set_page_used(size_t page) {
    memory_bitmap[page / 8] |= (1 << (page % 8));
}

void set_page_free(size_t page) {
    memory_bitmap[page / 8] &= ~(1 << (page % 8));
}

int is_page_used(size_t page) {
    return memory_bitmap[page / 8] & (1 << (page % 8));
}

void *allocate_page() {
    for (size_t i = 0; i < BITMAP_SIZE * 8; i++) {
        if (!is_page_used(i)) {
            set_page_used(i);
            print("Allocated page at index: ");
            print_hex(i);
            print("\n");
            return (void *)(i * PAGE_SIZE);
        }
    }
    print("No free pages available.\n");
    return NULL;
}

void free_page(void *ptr) {
    size_t page = (size_t)ptr / PAGE_SIZE;
    set_page_free(page);
}

void memory_init() {
    for (size_t i = 0; i < BITMAP_SIZE; i++) {
        memory_bitmap[i] = 0;
    }
    print("Memory initialized. All pages set to free.\n");
    print("BITMAP_SIZE: ");
    print_hex(BITMAP_SIZE);
    print("\n");
}
