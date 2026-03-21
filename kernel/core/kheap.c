#include "kheap.h"
#include "string.h"
#include "video.h"
#include "kernel.h"

/* Global heap state */
static kheap_state_t g_heap;

/* Block header size */
#define HEADER_SIZE sizeof(kheap_block_t)

/* Align size up to KHEAP_BLOCK_SIZE */
static inline size_t align_up(size_t size) {
    return (size + KHEAP_BLOCK_SIZE - 1) & ~(KHEAP_BLOCK_SIZE - 1);
}

/* Get pointer to block data (after header) */
static inline void* block_to_data(kheap_block_t* block) {
    return (void*)((uint8_t*)block + HEADER_SIZE);
}

/* Get pointer to block header from data pointer */
static inline kheap_block_t* data_to_block(void* ptr) {
    return (kheap_block_t*)((uint8_t*)ptr - HEADER_SIZE);
}

/* Check if a pointer is within heap bounds */
static inline int is_in_heap(void* ptr) {
    return ptr >= g_heap.heap_start && ptr < g_heap.heap_end;
}

/* Remove block from a list */
static void list_remove(kheap_block_t** list, kheap_block_t* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        *list = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
    block->next = NULL;
    block->prev = NULL;
}

/* Add block to the front of a list */
static void list_push_front(kheap_block_t** list, kheap_block_t* block) {
    block->next = *list;
    block->prev = NULL;
    if (*list) {
        (*list)->prev = block;
    }
    *list = block;
}

/* Find best fit block in free list */
static kheap_block_t* find_best_fit(size_t size) {
    kheap_block_t* best = NULL;
    kheap_block_t* current = g_heap.free_list;
    
    while (current) {
        if (current->size >= size) {
            if (!best || current->size < best->size) {
                best = current;
                /* Perfect fit - no need to continue */
                if (best->size == size) {
                    break;
                }
            }
        }
        current = current->next;
    }
    
    return best;
}

/* Split a block if it's large enough */
static void split_block(kheap_block_t* block, size_t size) {
    size_t remaining = block->size - size - HEADER_SIZE;
    
    /* Only split if remaining space is useful */
    if (remaining >= KHEAP_MIN_BLOCK) {
        kheap_block_t* new_block = (kheap_block_t*)((uint8_t*)block + HEADER_SIZE + size);
        new_block->magic = KHEAP_MAGIC;
        new_block->flags = KHEAP_FLAG_FREE;
        new_block->size = remaining;
        
        /* Insert new block after the current one in free list */
        new_block->next = block->next;
        new_block->prev = block;
        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;
        
        /* Resize current block */
        block->size = size;
    }
}

/* Coalesce with adjacent free blocks */
static void coalesce(kheap_block_t* block) {
    /* Try to coalesce with next block */
    if (block->next && block->next->flags == KHEAP_FLAG_FREE) {
        kheap_block_t* next = block->next;
        list_remove(&g_heap.free_list, next);
        block->size += HEADER_SIZE + next->size;
        block->next = next->next;
        if (next->next) {
            next->next->prev = block;
        }
    }
    
    /* Try to coalesce with prev block */
    if (block->prev && block->prev->flags == KHEAP_FLAG_FREE) {
        kheap_block_t* prev = block->prev;
        list_remove(&g_heap.free_list, prev);
        prev->size += HEADER_SIZE + block->size;
        prev->next = block->next;
        if (block->next) {
            block->next->prev = prev;
        }
        /* Block is now part of prev, so prev is the coalesced block */
    }
}

kheap_state_t* kheap_get_state(void) {
    return &g_heap;
}

int kheap_init(void* start, size_t size) {
    if (g_heap.initialized) {
        return 0; /* Already initialized */
    }
    
    if (!start || size < KHEAP_MIN_BLOCK * 2) {
        return -1;
    }
    
    /* Align start address */
    uint32_t aligned_start = ((uint32_t)start + KHEAP_BLOCK_SIZE - 1) & ~(KHEAP_BLOCK_SIZE - 1);
    size_t aligned_size = size - (aligned_start - (uint32_t)start);
    aligned_size &= ~(KHEAP_BLOCK_SIZE - 1);
    
    /* Initialize heap state */
    g_heap.heap_start = (void*)aligned_start;
    g_heap.heap_end = (void*)(aligned_start + aligned_size);
    g_heap.total_size = aligned_size;
    g_heap.total_allocated = 0;
    g_heap.total_freed = 0;
    g_heap.num_allocs = 0;
    g_heap.num_frees = 0;
    g_heap.used_list = NULL;
    
    /* Create initial free block spanning entire heap */
    kheap_block_t* initial = (kheap_block_t*)aligned_start;
    initial->magic = KHEAP_MAGIC;
    initial->flags = KHEAP_FLAG_FREE;
    initial->size = aligned_size - HEADER_SIZE;
    initial->next = NULL;
    initial->prev = NULL;
    
    g_heap.free_list = initial;
    g_heap.initialized = 1;
    
    debug_print("kheap: initialized at 0x");
    debug_print_hex(aligned_start);
    debug_print(" size ");
    debug_print_hex(aligned_size);
    debug_print(" bytes\n");
    
    return 0;
}

void* kmalloc(size_t size) {
    if (!g_heap.initialized || size == 0) {
        return NULL;
    }
    
    /* Align size */
    size = align_up(size);
    
    /* Find best fit block */
    kheap_block_t* block = find_best_fit(size);
    if (!block) {
        debug_print("kheap: out of memory for size ");
        debug_print_hex(size);
        debug_print("\n");
        return NULL;
    }
    
    /* Remove from free list */
    list_remove(&g_heap.free_list, block);
    
    /* Split if block is much larger */
    split_block(block, size);
    
    /* Mark as used */
    block->flags = KHEAP_FLAG_USED;
    
    /* Add to used list */
    list_push_front(&g_heap.used_list, block);
    
    /* Update stats */
    g_heap.total_allocated += block->size;
    g_heap.num_allocs++;
    
    return block_to_data(block);
}

void kfree(void* ptr) {
    if (!ptr || !g_heap.initialized) {
        return;
    }
    
    /* Check pointer is in heap */
    if (!is_in_heap(ptr)) {
        debug_print("kheap: invalid free pointer\n");
        return;
    }
    
    /* Get block header */
    kheap_block_t* block = data_to_block(ptr);
    
    /* Validate block */
    if (block->magic != KHEAP_MAGIC) {
        debug_print("kheap: bad magic on free - heap corruption!\n");
        return;
    }
    
    if (block->flags == KHEAP_FLAG_FREE) {
        debug_print("kheap: double free detected!\n");
        return;
    }
    
    /* Remove from used list */
    list_remove(&g_heap.used_list, block);
    
    /* Mark as free */
    block->flags = KHEAP_FLAG_FREE;
    
    /* Update stats */
    g_heap.total_allocated -= block->size;
    g_heap.total_freed += block->size;
    g_heap.num_frees++;
    
    /* Coalesce with adjacent free blocks */
    coalesce(block);
    
    /* Add to free list */
    list_push_front(&g_heap.free_list, block);
}

void* krealloc(void* ptr, size_t size) {
    /* realloc(NULL, size) is equivalent to malloc(size) */
    if (!ptr) {
        return kmalloc(size);
    }
    
    /* realloc(ptr, 0) is equivalent to free(ptr) */
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    kheap_block_t* block = data_to_block(ptr);
    
    /* Validate block */
    if (block->magic != KHEAP_MAGIC) {
        debug_print("kheap: bad magic on realloc\n");
        return NULL;
    }
    
    size = align_up(size);
    
    /* If size is similar, return same pointer */
    if (size <= block->size && block->size - size < KHEAP_MIN_BLOCK) {
        return ptr;
    }
    
    /* If new size is smaller, try to split */
    if (size < block->size) {
        split_block(block, size);
        return ptr;
    }
    
    /* Need to allocate new block */
    void* new_ptr = kmalloc(size);
    if (!new_ptr) {
        return NULL;
    }
    
    /* Copy old data */
    size_t copy_size = block->size < size ? block->size : size;
    memcpy(new_ptr, ptr, copy_size);
    
    /* Free old block */
    kfree(ptr);
    
    return new_ptr;
}

void* kcalloc(size_t num, size_t size) {
    size_t total = num * size;
    
    /* Check for overflow */
    if (num != 0 && total / num != size) {
        return NULL;
    }
    
    void* ptr = kmalloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    
    return ptr;
}

int kheap_check_integrity(void) {
    if (!g_heap.initialized) {
        return 0;
    }
    
    /* Check free list */
    kheap_block_t* block = g_heap.free_list;
    while (block) {
        if (block->magic != KHEAP_MAGIC) {
            debug_print("kheap: corruption in free list\n");
            return -1;
        }
        if (block->flags != KHEAP_FLAG_FREE) {
            debug_print("kheap: used block in free list\n");
            return -1;
        }
        if (!is_in_heap(block)) {
            debug_print("kheap: block outside heap\n");
            return -1;
        }
        block = block->next;
    }
    
    /* Check used list */
    block = g_heap.used_list;
    while (block) {
        if (block->magic != KHEAP_MAGIC) {
            debug_print("kheap: corruption in used list\n");
            return -1;
        }
        if (block->flags != KHEAP_FLAG_USED) {
            debug_print("kheap: free block in used list\n");
            return -1;
        }
        if (!is_in_heap(block)) {
            debug_print("kheap: block outside heap\n");
            return -1;
        }
        block = block->next;
    }
    
    return 0;
}

void kheap_dump(void) {
    debug_print("\n=== Kernel Heap Dump ===\n");
    debug_print("Start: 0x");
    debug_print_hex((uint32_t)g_heap.heap_start);
    debug_print(" End: 0x");
    debug_print_hex((uint32_t)g_heap.heap_end);
    debug_print("\nTotal: ");
    debug_print_hex(g_heap.total_size);
    debug_print(" Allocated: ");
    debug_print_hex(g_heap.total_allocated);
    debug_print("\nAllocs: ");
    debug_print_hex(g_heap.num_allocs);
    debug_print(" Frees: ");
    debug_print_hex(g_heap.num_frees);
    debug_print("\n");
    
    debug_print("Free blocks:\n");
    kheap_block_t* block = g_heap.free_list;
    while (block) {
        debug_print("  0x");
        debug_print_hex((uint32_t)block);
        debug_print(" size ");
        debug_print_hex(block->size);
        debug_print("\n");
        block = block->next;
    }
    
    debug_print("Used blocks:\n");
    block = g_heap.used_list;
    while (block) {
        debug_print("  0x");
        debug_print_hex((uint32_t)block);
        debug_print(" size ");
        debug_print_hex(block->size);
        debug_print("\n");
        block = block->next;
    }
    debug_print("========================\n");
}