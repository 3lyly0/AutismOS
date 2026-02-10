#include "process.h"
#include "memory.h"
#include "task.h"
#include "string.h"
#include "kernel.h"
#include "video.h"
#include "usermode.h"

#define MAX_PROCESSES 32
#define USER_STACK_SIZE 4096

static process_t* current_process = NULL;
static process_t* process_list_head = NULL;
static uint32 next_pid = 0;

// Get current process
process_t* process_get_current(void) {
    return current_process;
}

// Initialize process subsystem
void process_init(void) {
    current_process = NULL;
    process_list_head = NULL;
    next_pid = 0;
    print("Process subsystem initialized\n");
}

// Create a new process
process_t* process_create(void (*entry_point)(void), uint32 is_user_mode) {
    // Allocate process structure
    process_t* new_process = (process_t*)kmalloc(sizeof(process_t));
    if (!new_process) {
        kernel_panic("Failed to allocate process structure");
    }
    
    memset(new_process, 0, sizeof(process_t));
    new_process->pid = next_pid++;
    
    // Create page directory for this process
    new_process->page_dir = create_page_directory();
    if (!new_process->page_dir) {
        kfree(new_process);
        kernel_panic("Failed to create page directory for process");
    }
    
    // Create main thread for this process
    // Note: We'll create the task but manage it through the process
    new_process->main_thread = task_create(entry_point);
    if (!new_process->main_thread) {
        kfree(new_process);
        kernel_panic("Failed to create main thread for process");
    }
    
    // Link task back to process
    new_process->main_thread->process = new_process;
    
    // Add to process list (circular linked list)
    if (process_list_head == NULL) {
        // First process
        process_list_head = new_process;
        new_process->next = new_process; // Points to itself
        current_process = new_process;
    } else {
        // Insert at end of circular list
        process_t* last = process_list_head;
        while (last->next != process_list_head) {
            last = last->next;
        }
        last->next = new_process;
        new_process->next = process_list_head;
    }
    
    print("Process created: PID=");
    print_hex(new_process->pid);
    print(" at 0x");
    print_hex((uint32)new_process);
    print("\n");
    
    return new_process;
}

// Switch to a different process (changes address space)
void process_switch(process_t* next) {
    if (!next) {
        return;
    }
    
    if (current_process != next) {
        current_process = next;
        // Switch page directory (change CR3)
        switch_page_directory(next->page_dir);
    }
}
