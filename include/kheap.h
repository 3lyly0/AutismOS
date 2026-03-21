#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

/* Heap configuration */
#define KHEAP_BLOCK_SIZE   16
#define KHEAP_MIN_BLOCK    32

/* Block header flags */
#define KHEAP_FLAG_FREE    0x0
#define KHEAP_FLAG_USED    0x1
#define KHEAP_MAGIC        0xDEADBEEF

typedef struct kheap_block {
    uint32_t magic;
    uint32_t flags;
    size_t size;
    struct kheap_block* next;
    struct kheap_block* prev;
} kheap_block_t;

typedef struct {
    kheap_block_t* free_list;
    kheap_block_t* used_list;
    void* heap_start;
    void* heap_end;
    size_t total_size;
    size_t total_allocated;
    size_t total_freed;
    uint32_t num_allocs;
    uint32_t num_frees;
    uint8_t initialized;
} kheap_state_t;

int kheap_init(void* start, size_t size);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);
void* kcalloc(size_t num, size_t size);
kheap_state_t* kheap_get_state(void);
int kheap_check_integrity(void);
void kheap_dump(void);

#endif