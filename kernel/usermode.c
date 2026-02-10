#include "usermode.h"
#include "memory.h"
#include "video.h"
#include "string.h"
#include "kernel.h"

// Page flags from memory.h
#define PAGE_PRESENT    0x1
#define PAGE_WRITE      0x2
#define PAGE_USER       0x4

// User space allocator - simple bump allocator
static void* user_heap_current = NULL;

// Initialize user mode subsystem
void usermode_init(void) {
    user_heap_current = (void*)USER_SPACE_START;
    print("User mode subsystem initialized\n");
    print("User space: 0x");
    print_hex(USER_SPACE_START);
    print(" - 0x");
    print_hex(USER_SPACE_END);
    print("\n");
}

// Allocate user space memory
void* allocate_user_memory(uint32 size) {
    if (!user_heap_current) {
        kernel_panic("User mode not initialized");
    }
    
    // Align to page boundary
    uint32 aligned_size = (size + 0xFFF) & ~0xFFF;
    
    void* result = user_heap_current;
    user_heap_current = (void*)((uint32)user_heap_current + aligned_size);
    
    if ((uint32)user_heap_current > USER_SPACE_END) {
        kernel_panic("User space exhausted");
    }
    
    // Map pages as user-accessible using kernel page directory
    page_directory_t* kernel_dir = get_kernel_page_directory();
    
    // Allocate and map the pages with user permissions
    uint32 start_addr = (uint32)result;
    uint32 end_addr = (uint32)user_heap_current;
    
    for (uint32 addr = start_addr; addr < end_addr; addr += 0x1000) {
        // Map to physical address (identity mapping for now)
        map_page_in_directory(kernel_dir, addr, addr, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }
    
    // Flush TLB for the affected pages
    for (uint32 addr = start_addr; addr < end_addr; addr += 0x1000) {
        asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
    }
    
    // Clear the allocated memory (full aligned size to prevent memory leaks)
    memset(result, 0, aligned_size);
    
    return result;
}

// Switch to user mode
// This function sets up the stack and uses iret to switch to Ring 3
void switch_to_user_mode(uint32 entry_point, uint32 user_stack) {
    print("Switching to user mode...\n");
    print("Entry point: 0x");
    print_hex(entry_point);
    print("\nUser stack: 0x");
    print_hex(user_stack);
    print("\n");
    
    // Disable interrupts during switch
    asm volatile("cli");
    
    // Set up user mode segments
    // User data segment selector = 0x23 (GDT entry 4, RPL=3)
    // User code segment selector = 0x1B (GDT entry 3, RPL=3)
    
    asm volatile(
        // Set up data segments for user mode
        "mov $0x23, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        
        // Push stack segment (SS) - user data segment
        "pushl $0x23\n"
        
        // Push stack pointer (ESP)
        "pushl %0\n"
        
        // Push EFLAGS with interrupts enabled
        "pushfl\n"
        "popl %%eax\n"
        "orl $0x200, %%eax\n"  // Set IF flag
        "pushl %%eax\n"
        
        // Push code segment (CS) - user code segment
        "pushl $0x1B\n"
        
        // Push instruction pointer (EIP)
        "pushl %1\n"
        
        // Execute iret to switch to Ring 3
        "iret\n"
        :
        : "r"(user_stack), "r"(entry_point)
        : "eax"
    );
    
    // Should never reach here
    kernel_panic("Failed to switch to user mode");
}
