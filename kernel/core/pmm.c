#include "pmm.h"
#include "string.h"
#include "video.h"
#include "kernel.h"

/* Global PMM state */
static struct {
    uint8_t* bitmap;          /* Bitmap: 1 = used, 0 = free */
    uint32_t total_frames;    /* Total number of frames */
    uint32_t free_frames;     /* Number of free frames */
    uint32_t used_frames;     /* Number of used frames */
    uint32_t bitmap_size;     /* Size of bitmap in bytes */
    uint8_t initialized;      /* Is PMM initialized? */
} g_pmm;

/* Bitmap manipulation macros */
#define BITMAP_INDEX(frame) ((frame) / 8)
#define BITMAP_OFFSET(frame) ((frame) % 8)
#define BITMAP_GET(frame) (g_pmm.bitmap[BITMAP_INDEX(frame)] & (1 << BITMAP_OFFSET(frame)))
#define BITMAP_SET(frame) (g_pmm.bitmap[BITMAP_INDEX(frame)] |= (1 << BITMAP_OFFSET(frame)))
#define BITMAP_CLEAR(frame) (g_pmm.bitmap[BITMAP_INDEX(frame)] &= ~(1 << BITMAP_OFFSET(frame)))

int pmm_init(uint32_t mem_size, void* bitmap) {
    if (g_pmm.initialized) {
        return 0; /* Already initialized */
    }
    
    /* Calculate number of frames */
    g_pmm.total_frames = mem_size / PMM_PAGE_SIZE;
    if (g_pmm.total_frames == 0) {
        return -1;
    }
    
    /* Calculate bitmap size (one bit per frame) */
    g_pmm.bitmap_size = (g_pmm.total_frames + 7) / 8;
    
    /* Use provided bitmap or allocate from kernel */
    if (bitmap) {
        g_pmm.bitmap = (uint8_t*)bitmap;
    } else {
        /* This would need a simple allocator before PMM is ready */
        debug_print("PMM: No bitmap provided, cannot initialize\n");
        return -1;
    }
    
    /* Clear bitmap - all frames initially free */
    memset(g_pmm.bitmap, 0, g_pmm.bitmap_size);
    
    g_pmm.free_frames = g_pmm.total_frames;
    g_pmm.used_frames = 0;
    g_pmm.initialized = 1;
    
    debug_print("PMM: initialized with ");
    debug_print_hex(g_pmm.total_frames);
    debug_print(" frames (");
    debug_print_hex(mem_size / 1024);
    debug_print(" KB)\n");
    
    return 0;
}

void pmm_mark_used(uint32_t frame) {
    if (frame >= g_pmm.total_frames) {
        return;
    }
    
    if (!BITMAP_GET(frame)) {
        BITMAP_SET(frame);
        g_pmm.used_frames++;
        g_pmm.free_frames--;
    }
}

void pmm_mark_free(uint32_t frame) {
    if (frame >= g_pmm.total_frames) {
        return;
    }
    
    if (BITMAP_GET(frame)) {
        BITMAP_CLEAR(frame);
        g_pmm.used_frames--;
        g_pmm.free_frames++;
    }
}

void pmm_mark_range_used(uint32_t start, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        pmm_mark_used(start + i);
    }
}

void pmm_mark_range_free(uint32_t start, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        pmm_mark_free(start + i);
    }
}

uint32_t pmm_alloc_frame(void) {
    if (!g_pmm.initialized || g_pmm.free_frames == 0) {
        return 0;
    }
    
    /* Search for a free frame */
    for (uint32_t i = 1; i < g_pmm.total_frames; i++) { /* Skip frame 0 */
        if (!BITMAP_GET(i)) {
            BITMAP_SET(i);
            g_pmm.used_frames++;
            g_pmm.free_frames--;
            return i;
        }
    }
    
    return 0; /* No free frame found */
}

uint32_t pmm_alloc_frames(uint32_t count) {
    if (!g_pmm.initialized || g_pmm.free_frames < count) {
        return 0;
    }
    
    uint32_t consecutive = 0;
    uint32_t start_frame = 0;
    
    /* Search for contiguous free frames */
    for (uint32_t i = 1; i < g_pmm.total_frames && consecutive < count; i++) {
        if (!BITMAP_GET(i)) {
            if (consecutive == 0) {
                start_frame = i;
            }
            consecutive++;
        } else {
            consecutive = 0;
            start_frame = 0;
        }
    }
    
    /* Found enough contiguous frames */
    if (consecutive >= count) {
        for (uint32_t i = 0; i < count; i++) {
            BITMAP_SET(start_frame + i);
        }
        g_pmm.used_frames += count;
        g_pmm.free_frames -= count;
        return start_frame;
    }
    
    return 0; /* Not enough contiguous frames */
}

void pmm_free_frame(uint32_t frame) {
    pmm_mark_free(frame);
}

void pmm_free_frames(uint32_t start, uint32_t count) {
    pmm_mark_range_free(start, count);
}

int pmm_is_frame_free(uint32_t frame) {
    if (frame >= g_pmm.total_frames) {
        return 0;
    }
    return !BITMAP_GET(frame);
}

void pmm_get_stats(pmm_stats_t* stats) {
    if (!stats) {
        return;
    }
    
    stats->total_frames = g_pmm.total_frames;
    stats->free_frames = g_pmm.free_frames;
    stats->used_frames = g_pmm.used_frames;
    stats->reserved_frames = 0; /* Not tracked separately */
    stats->total_memory = (g_pmm.total_frames * PMM_PAGE_SIZE) / 1024;
    stats->free_memory = (g_pmm.free_frames * PMM_PAGE_SIZE) / 1024;
    stats->initialized = g_pmm.initialized;
}

void pmm_dump(void) {
    debug_print("\n=== Physical Memory Manager ===\n");
    debug_print("Total frames: ");
    debug_print_hex(g_pmm.total_frames);
    debug_print("\nFree frames: ");
    debug_print_hex(g_pmm.free_frames);
    debug_print("\nUsed frames: ");
    debug_print_hex(g_pmm.used_frames);
    debug_print("\nBitmap size: ");
    debug_print_hex(g_pmm.bitmap_size);
    debug_print(" bytes\n");
    
    /* Show first 64 frames as sample */
    debug_print("First 64 frames: ");
    for (int i = 0; i < 64 && i < (int)g_pmm.total_frames; i++) {
        if (i % 32 == 0) {
            debug_print("\n  ");
            debug_print_hex(i);
            debug_print(": ");
        }
        debug_print(BITMAP_GET(i) ? "1" : "0");
    }
    debug_print("\n================================\n");
}

uint32_t pmm_get_free_frames(uint32_t* frames, uint32_t count) {
    if (!frames || count == 0) {
        return 0;
    }
    
    uint32_t found = 0;
    for (uint32_t i = 1; i < g_pmm.total_frames && found < count; i++) {
        if (!BITMAP_GET(i)) {
            frames[found++] = i;
        }
    }
    
    return found;
}