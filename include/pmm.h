#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

/**
 * Physical Memory Manager
 * 
 * Manages physical memory using a bitmap allocator.
 * Tracks which physical frames are available for use.
 */

/* Page/frame size - 4KB */
#define PMM_PAGE_SIZE      4096
#define PMM_PAGE_SHIFT     12

/* Convert between address and frame number */
#define PMM_ADDR_TO_FRAME(addr)  ((uint32_t)(addr) >> PMM_PAGE_SHIFT)
#define PMM_FRAME_TO_ADDR(frame) ((uint32_t)(frame) << PMM_PAGE_SHIFT)

/* Memory region types */
typedef enum {
    PMM_REGION_FREE = 0,      /* Available for allocation */
    PMM_REGION_USED = 1,      /* In use by kernel/hardware */
    PMM_REGION_RESERVED = 2,  /* Reserved (BIOS, hardware) */
    PMM_REGION_KERNEL = 3     /* Kernel code/data */
} pmm_region_type_t;

/* Memory region descriptor */
typedef struct {
    uint64_t base;            /* Base address */
    uint64_t length;          /* Length in bytes */
    pmm_region_type_t type;   /* Region type */
} pmm_region_t;

/* PMM statistics */
typedef struct {
    uint32_t total_frames;    /* Total number of frames */
    uint32_t free_frames;     /* Number of free frames */
    uint32_t used_frames;     /* Number of used frames */
    uint32_t reserved_frames; /* Number of reserved frames */
    uint32_t total_memory;    /* Total memory in KB */
    uint32_t free_memory;     /* Free memory in KB */
    uint8_t initialized;      /* Is PMM initialized? */
} pmm_stats_t;

/* ============== Public API ============== */

/**
 * Initialize the physical memory manager
 * 
 * @param mem_size   Total physical memory size in bytes
 * @param bitmap     Pointer to bitmap storage (or NULL to allocate)
 * @return           0 on success, -1 on failure
 */
int pmm_init(uint32_t mem_size, void* bitmap);

/**
 * Mark a physical frame as used
 * 
 * @param frame      Frame number to mark
 */
void pmm_mark_used(uint32_t frame);

/**
 * Mark a physical frame as free
 * 
 * @param frame      Frame number to free
 */
void pmm_mark_free(uint32_t frame);

/**
 * Mark a range of frames as used
 * 
 * @param start      Starting frame number
 * @param count      Number of frames
 */
void pmm_mark_range_used(uint32_t start, uint32_t count);

/**
 * Mark a range of frames as free
 * 
 * @param start      Starting frame number
 * @param count      Number of frames
 */
void pmm_mark_range_free(uint32_t start, uint32_t count);

/**
 * Allocate a single physical frame
 * 
 * @return           Physical frame number, or 0 if none available
 */
uint32_t pmm_alloc_frame(void);

/**
 * Allocate contiguous physical frames
 * 
 * @param count      Number of frames to allocate
 * @return           Starting frame number, or 0 if not available
 */
uint32_t pmm_alloc_frames(uint32_t count);

/**
 * Free a single physical frame
 * 
 * @param frame      Frame number to free
 */
void pmm_free_frame(uint32_t frame);

/**
 * Free contiguous physical frames
 * 
 * @param start      Starting frame number
 * @param count      Number of frames to free
 */
void pmm_free_frames(uint32_t start, uint32_t count);

/**
 * Check if a frame is free
 * 
 * @param frame      Frame number to check
 * @return           1 if free, 0 if used
 */
int pmm_is_frame_free(uint32_t frame);

/**
 * Get PMM statistics
 * 
 * @param stats      Pointer to pmm_stats_t to fill
 */
void pmm_get_stats(pmm_stats_t* stats);

/**
 * Dump PMM state for debugging
 */
void pmm_dump(void);

/**
 * Get first N free frames (for debugging)
 * 
 * @param frames     Array to store frame numbers
 * @param count      Maximum number of frames to find
 * @return           Number of frames found
 */
uint32_t pmm_get_free_frames(uint32_t* frames, uint32_t count);

#endif /* PMM_H */