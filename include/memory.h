#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include "multiboot.h"

// Page directory type (opaque pointer)
typedef struct page_directory page_directory_t;

void memory_init(multiboot_info_t *mbi);
void *allocate_page();
void free_page(void *ptr);

void paging_init();
void paging_enable();

void *kmalloc(size_t size);
void kfree(void *ptr);

// Page directory management for processes
page_directory_t* create_page_directory(void);
void switch_page_directory(page_directory_t* dir);
page_directory_t* get_kernel_page_directory(void);
void map_page_in_directory(page_directory_t* dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

#endif