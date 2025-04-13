#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

void memory_init();
void *allocate_page();
void free_page(void *ptr);

#endif