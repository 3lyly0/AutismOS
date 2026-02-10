#include "shm.h"
#include "memory.h"
#include "kernel.h"
#include "string.h"
#include "process.h"

// Maximum shared memory regions
#define MAX_SHM_REGIONS 32

// Shared memory region list
static shm_region_t shm_regions[MAX_SHM_REGIONS];
static uint32 next_shm_id = 1;

// Initialize shared memory subsystem
void shm_init(void) {
    memset(shm_regions, 0, sizeof(shm_regions));
    next_shm_id = 1;
}

// Find a free shared memory region slot
static shm_region_t* shm_alloc_region(void) {
    for (int i = 0; i < MAX_SHM_REGIONS; i++) {
        if (shm_regions[i].id == 0) {
            return &shm_regions[i];
        }
    }
    return NULL;
}

// Find shared memory region by ID
shm_region_t* shm_find_by_id(uint32 shm_id) {
    for (int i = 0; i < MAX_SHM_REGIONS; i++) {
        if (shm_regions[i].id == shm_id) {
            return &shm_regions[i];
        }
    }
    return NULL;
}

// Create a new shared memory region
// Returns: shared memory ID, or 0 on failure
uint32 shm_create(uint32 size) {
    if (size == 0 || size > 0x400000) {  // Max 4MB per region
        return 0;
    }
    
    // Allocate region structure
    shm_region_t* region = shm_alloc_region();
    if (!region) {
        return 0;
    }
    
    // Round size up to page boundary
    uint32 page_size = 4096;
    uint32 aligned_size = (size + page_size - 1) & ~(page_size - 1);
    
    // Allocate contiguous memory from kernel heap instead of page allocator
    // This gives us the required contiguous block for larger allocations
    void* kernel_vaddr = kmalloc(aligned_size);
    if (!kernel_vaddr) {
        return 0;
    }
    
    // Initialize the region
    region->id = next_shm_id++;
    region->size = aligned_size;
    region->kernel_vaddr = kernel_vaddr;
    region->phys_addr = (uint32)kernel_vaddr;  // Identity mapped
    region->owner_pid = process_get_current() ? process_get_current()->pid : 0;
    region->ref_count = 0;
    region->next = NULL;
    
    // Zero the memory
    memset(kernel_vaddr, 0, aligned_size);
    
    return region->id;
}

// Map a shared memory region into the current process
// Returns: 0 on success, -1 on failure
int shm_map(uint32 shm_id, void** vaddr_out) {
    if (!vaddr_out) {
        return -1;
    }
    
    shm_region_t* region = shm_find_by_id(shm_id);
    if (!region) {
        return -1;
    }
    
    // For this simple implementation, we return the kernel virtual address
    // In a real system, we would map it into the process's address space
    *vaddr_out = region->kernel_vaddr;
    region->ref_count++;
    
    return 0;
}

// Unmap a shared memory region from the current process
// Returns: 0 on success, -1 on failure
int shm_unmap(uint32 shm_id) {
    shm_region_t* region = shm_find_by_id(shm_id);
    if (!region) {
        return -1;
    }
    
    if (region->ref_count > 0) {
        region->ref_count--;
    }
    
    // If no more references, we could free the region
    // For now, we keep it alive
    
    return 0;
}
