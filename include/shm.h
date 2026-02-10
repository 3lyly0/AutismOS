#ifndef SHM_H
#define SHM_H

#include "types.h"

// Shared memory region structure
typedef struct shm_region {
    uint32 id;                    // Shared memory region ID
    uint32 size;                  // Size in bytes
    void* kernel_vaddr;           // Kernel virtual address
    uint32 phys_addr;             // Physical address of the region
    uint32 owner_pid;             // PID of the process that created it
    uint32 ref_count;             // Number of processes mapping this region
    struct shm_region* next;      // Next region in list
} shm_region_t;

// Framebuffer structure for shared rendering
typedef struct framebuffer {
    uint32 width;
    uint32 height;
    uint32 pitch;                 // Bytes per row
    uint32* pixels;               // Pointer to pixel data
} framebuffer_t;

// Shared memory management functions
void shm_init(void);
uint32 shm_create(uint32 size);
int shm_map(uint32 shm_id, void** vaddr_out);
int shm_unmap(uint32 shm_id);
shm_region_t* shm_find_by_id(uint32 shm_id);

#endif
