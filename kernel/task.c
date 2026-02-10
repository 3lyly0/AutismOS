#include "task.h"
#include "memory.h"
#include "string.h"
#include "kernel.h"
#include "video.h"

#define TASK_STACK_SIZE 4096
#define MAX_TASKS 32
#define TASK_YIELD_DELAY 100000  // Busy-wait iterations before yielding

static task_t* current_task = NULL;
static task_t* task_list_head = NULL;
static uint32 next_task_id = 0;

// Get current task
task_t* task_get_current(void) {
    return current_task;
}

// Scheduler tick - called from timer IRQ
// Returns the ESP to use (either current or switched task)
uint32 task_scheduler_tick(uint32 current_esp) {
    // Increment timer ticks (for kernel use)
    extern volatile uint64 g_timer_ticks;
    g_timer_ticks++;
    
    // Send EOI to PIC
    extern void pic8259_eoi(int irq);
    pic8259_eoi(32); // IRQ0
    
    // If no tasks or only one task, return current ESP
    if (!current_task || !current_task->next || current_task->next == current_task) {
        return current_esp;
    }
    
    // Save current task's ESP
    current_task->esp = current_esp;
    current_task->state = TASK_READY;
    
    // Get next task (round-robin)
    task_t* next_task = current_task->next;
    task_t* start_task = current_task;
    
    // Skip blocked tasks if any (basic implementation)
    while (next_task->state != TASK_READY && next_task->state != TASK_RUNNING) {
        next_task = next_task->next;
        if (next_task == start_task) break; // Went full circle, stay on current
    }
    
    // Switch to next task
    current_task = next_task;
    current_task->state = TASK_RUNNING;
    
    // Return new task's ESP
    return current_task->esp;
}

// Initialize task system
void task_init(void) {
    current_task = NULL;
    task_list_head = NULL;
    next_task_id = 0;
    print("Task system initialized\n");
}

// Create a new task
task_t* task_create(void (*entry_point)(void)) {
    // Allocate task structure
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    if (!new_task) {
        kernel_panic("Failed to allocate task structure");
    }
    
    // Allocate stack
    void* stack = kmalloc(TASK_STACK_SIZE);
    if (!stack) {
        kfree(new_task);
        kernel_panic("Failed to allocate task stack");
    }
    
    // Initialize task structure
    memset(new_task, 0, sizeof(task_t));
    new_task->id = next_task_id++;
    new_task->state = TASK_READY;
    
    // Set up stack
    // Stack grows downward, so we start at the top
    uint32* stack_top = (uint32*)((uint32)stack + TASK_STACK_SIZE);
    
    // Push initial values onto the stack to simulate interrupt context
    // When we context switch to this task, it will pop these values
    
    // Push EFLAGS (with interrupts enabled)
    stack_top--;
    *stack_top = 0x202; // IF flag set (interrupts enabled)
    
    // Push CS (code segment)
    stack_top--;
    *stack_top = 0x08; // Kernel code segment
    
    // Push EIP (entry point)
    stack_top--;
    *stack_top = (uint32)entry_point;
    
    // Push error code and interrupt number (not used, but maintain consistency)
    stack_top--;
    *stack_top = 0; // error code
    stack_top--;
    *stack_top = 0; // int_no
    
    // Push general purpose registers (pusha order)
    stack_top--;
    *stack_top = 0; // EAX
    stack_top--;
    *stack_top = 0; // ECX
    stack_top--;
    *stack_top = 0; // EDX
    stack_top--;
    *stack_top = 0; // EBX
    stack_top--;
    *stack_top = 0; // ESP (ignored by popa, but maintains interrupt frame format)
    stack_top--;
    *stack_top = 0; // EBP
    stack_top--;
    *stack_top = 0; // ESI
    stack_top--;
    *stack_top = 0; // EDI
    
    // Push DS
    stack_top--;
    *stack_top = 0x10; // Kernel data segment
    
    // Set ESP to current stack position
    new_task->esp = (uint32)stack_top;
    new_task->ebp = 0;
    new_task->eip = (uint32)entry_point;
    
    // Add to task list (circular linked list)
    if (task_list_head == NULL) {
        // First task
        task_list_head = new_task;
        new_task->next = new_task; // Points to itself
        current_task = new_task;
    } else {
        // Insert at end of circular list
        task_t* last = task_list_head;
        while (last->next != task_list_head) {
            last = last->next;
        }
        last->next = new_task;
        new_task->next = task_list_head;
    }
    
    return new_task;
}
