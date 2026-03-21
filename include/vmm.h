#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>

/**
 * Virtual Memory Manager
 * 
 * Manages virtual address spaces for processes.
 * Each process has its own page directory and virtual memory regions.
 */

/* Page sizes */
#define VMM_PAGE_SIZE       4096
#define VMM_PAGE_SHIFT      12
#define VMM_PAGE_MASK       0xFFFFF000

/* Page directory/table indices */
#define VMM_DIR_INDEX(addr) (((uint32_t)(addr) >> 22) & 0x3FF)
#define VMM_TABLE_INDEX(addr) (((uint32_t)(addr) >> 12) & 0x3FF)
#define VMM_OFFSET(addr)    ((uint32_t)(addr) & 0xFFF)

/* Page table entry flags */
#define VMM_FLAG_PRESENT    0x001
#define VMM_FLAG_WRITABLE   0x002
#define VMM_FLAG_USER       0x004
#define VMM_FLAG_WRITETHRU  0x008
#define VMM_FLAG_CACHE_DIS  0x010
#define VMM_FLAG_ACCESSED   0x020
#define VMM_FLAG_DIRTY      0x040
#define VMM_FLAG_4MB        0x080
#define VMM_FLAG_GLOBAL     0x100
#define VMM_FLAG_COW        0x200   /* Custom: Copy-on-write */

/* Default protection flags */
#define VMM_PROT_NONE       0
#define VMM_PROT_READ       VMM_FLAG_PRESENT
#define VMM_PROT_WRITE      (VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE)
#define VMM_PROT_EXEC       VMM_FLAG_PRESENT
#define VMM_PROT_USER       (VMM_FLAG_PRESENT | VMM_FLAG_USER)
#define VMM_PROT_RWX        (VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE | VMM_FLAG_USER)

/* Virtual memory area types */
typedef enum {
    VMA_TYPE_FREE = 0,
    VMA_TYPE_CODE,          /* Process code segment */
    VMA_TYPE_DATA,          /* Process data segment */
    VMA_TYPE_STACK,         /* Process stack */
    VMA_TYPE_HEAP,          /* Process heap */
    VMA_TYPE_MMAP,          /* Memory-mapped file/region */
    VMA_TYPE_SHARED         /* Shared memory */
} vma_type_t;

/* Virtual Memory Area structure */
typedef struct vma {
    uint32_t start;         /* Start virtual address */
    uint32_t end;           /* End virtual address (exclusive) */
    uint32_t flags;         /* Protection flags */
    vma_type_t type;        /* Type of region */
    struct vma* next;       /* Next VMA in list */
} vma_t;

/* Page directory structure (1024 entries) */
typedef struct {
    uint32_t entries[1024];
    uint32_t* tables[1024]; /* Virtual addresses of page tables */
    vma_t* vma_list;        /* List of VMAs for this space */
    uint32_t ref_count;     /* Reference count for sharing */
} page_directory_t;

/* VMM statistics */
typedef struct {
    uint32_t total_pages;       /* Total mapped pages */
    uint32_t user_pages;        /* User-space pages */
    uint32_t kernel_pages;      /* Kernel pages */
    uint32_t vma_count;         /* Number of VMAs */
    uint8_t initialized;        /* Is VMM initialized? */
} vmm_stats_t;

/* ============== Public API ============== */

/**
 * Initialize the virtual memory manager
 * Sets up kernel page directory
 * 
 * @return           0 on success, -1 on failure
 */
int vmm_init(void);

/**
 * Create a new page directory
 * Copies kernel mappings to new directory
 * 
 * @return           Pointer to new page directory, or NULL on failure
 */
page_directory_t* vmm_create_directory(void);

/**
 * Destroy a page directory
 * Frees all page tables and VMAs
 * 
 * @param dir        Page directory to destroy
 */
void vmm_destroy_directory(page_directory_t* dir);

/**
 * Switch to a different page directory
 * 
 * @param dir        Page directory to switch to
 */
void vmm_switch_directory(page_directory_t* dir);

/**
 * Get current page directory
 * 
 * @return           Pointer to current page directory
 */
page_directory_t* vmm_get_current_directory(void);

/**
 * Map a virtual page to a physical frame
 * 
 * @param dir        Page directory (NULL = current)
 * @param virt       Virtual address
 * @param phys       Physical address
 * @param flags      Page flags
 * @return           0 on success, -1 on failure
 */
int vmm_map_page(page_directory_t* dir, uint32_t virt, uint32_t phys, uint32_t flags);

/**
 * Unmap a virtual page
 * 
 * @param dir        Page directory (NULL = current)
 * @param virt       Virtual address
 */
void vmm_unmap_page(page_directory_t* dir, uint32_t virt);

/**
 * Get physical address for a virtual address
 * 
 * @param dir        Page directory (NULL = current)
 * @param virt       Virtual address
 * @param phys_out   Output physical address
 * @return           1 if mapped, 0 if not mapped
 */
int vmm_get_physical(page_directory_t* dir, uint32_t virt, uint32_t* phys_out);

/**
 * Check if a virtual address is mapped
 * 
 * @param dir        Page directory (NULL = current)
 * @param virt       Virtual address
 * @return           1 if mapped, 0 if not
 */
int vmm_is_mapped(page_directory_t* dir, uint32_t virt);

/**
 * Allocate and map a page at virtual address
 * 
 * @param dir        Page directory (NULL = current)
 * @param virt       Virtual address
 * @param flags      Page flags
 * @return           0 on success, -1 on failure
 */
int vmm_alloc_page(page_directory_t* dir, uint32_t virt, uint32_t flags);

/**
 * Allocate and map a range of pages
 * 
 * @param dir        Page directory (NULL = current)
 * @param virt       Start virtual address
 * @param count      Number of pages
 * @param flags      Page flags
 * @return           0 on success, -1 on failure
 */
int vmm_alloc_pages(page_directory_t* dir, uint32_t virt, uint32_t count, uint32_t flags);

/**
 * Free and unmap a range of pages
 * 
 * @param dir        Page directory (NULL = current)
 * @param virt       Start virtual address
 * @param count      Number of pages
 */
void vmm_free_pages(page_directory_t* dir, uint32_t virt, uint32_t count);

/* ============== VMA Management ============== */

/**
 * Create a VMA in a page directory
 * 
 * @param dir        Page directory
 * @param start      Start virtual address
 * @param size       Size in bytes
 * @param flags      Protection flags
 * @param type       VMA type
 * @return           Pointer to VMA, or NULL on failure
 */
vma_t* vmm_create_vma(page_directory_t* dir, uint32_t start, uint32_t size, 
                      uint32_t flags, vma_type_t type);

/**
 * Find a free region of the given size
 * 
 * @param dir        Page directory
 * @param size       Size in bytes
 * @param hint       Hint address (0 = any)
 * @return           Start address, or 0 if no space
 */
uint32_t vmm_find_free_region(page_directory_t* dir, uint32_t size, uint32_t hint);

/**
 * Dump VMM state for debugging
 */
void vmm_dump(void);

/**
 * Get VMM statistics
 * 
 * @param stats      Pointer to vmm_stats_t to fill
 */
void vmm_get_stats(vmm_stats_t* stats);

#endif /* VMM_H */