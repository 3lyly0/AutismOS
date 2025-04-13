#include "memory.h"
#include "types.h"
#include "string.h"
#include "video.h"
#include "io_ports.h"


#define PAGE_SIZE 4096
#define MEMORY_SIZE 0x1000000
#define BITMAP_SIZE (MEMORY_SIZE / PAGE_SIZE / 8)
#define TOTAL_PAGES (MEMORY_SIZE / PAGE_SIZE)

static uint8_t memory_bitmap[BITMAP_SIZE];



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
            print("Found free page at index: ");
            print_hex(i);
            print("\n");

            set_page_used(i);
            return (void *)(i * PAGE_SIZE);
        }
    }
    print("No free pages found in get_free_page.\n");
    return NULL;
}

void *allocate_page() {
    if (no_free_pages()) {
        print("No free pages available.\n");
        return NULL;
    }

    void *page = get_free_page();
    if (page == NULL) {
        print("Failed to get a free page.\n");
    } else {
        print("Allocated page at: ");
        print_hex((uint32_t)page);
        print("\n");
    }

    return page;
}

void free_page(void *ptr) {
    size_t page = (size_t)ptr / PAGE_SIZE;
    set_page_free(page);
}

void memory_init() {
    memset(memory_bitmap, 0, BITMAP_SIZE);

    for (int i = 0; i < TOTAL_PAGES; i++) {
        set_page_free(i);
    }
    print("Memory initialized. All pages set to free.\n");
    print("BITMAP_SIZE: ");
    print_hex(BITMAP_SIZE);
    print("\n");
}