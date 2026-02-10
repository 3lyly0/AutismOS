#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include "multiboot.h"

void memory_init(multiboot_info_t *mbi);
void *allocate_page();
void free_page(void *ptr);

void paging_init();
void paging_enable();

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif