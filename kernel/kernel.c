#include <stdint.h>
#include "kernel.h"
#include "string.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "memory.h"
#include "multiboot.h"
#include "mouse.h"
#include "video.h"
#include "keyboard.h"
#include "io_ports.h"
#include "disk.h"
#include "sound.h"
#include "task.h"
#include "syscall.h"
#include "usermode.h"
#include "user_program.h"

// Global tick counter for timer
volatile uint64 g_timer_ticks = 0;

// Timer interrupt handler
// Note: Task switching is now handled in the assembly IRQ handler
void timer_interrupt_handler(REGISTERS *regs) {
    (void)regs; // Suppress unused parameter warning
    // Timer tick and task switching handled by task_scheduler_tick
    // which is called directly from irq_0_with_task_switch
}

// Halt the CPU forever
static inline void halt_cpu_forever() {
    for (;;) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}

// Kernel panic handler - never returns
void kernel_panic(const char* message) {
    // Disable interrupts immediately
    asm volatile("cli");
    
    // Print panic message
    print("\n\n*** KERNEL PANIC ***\n");
    print(message);
    print("\n");
    print("System halted.\n");
    
    // Halt CPU forever
    halt_cpu_forever();
}

// Helper function for halting CPU (used in idle loop)
static inline void halt_cpu() {
    asm volatile("hlt");
}

// Idle task - runs when no other task is ready
void idle_task(void) {
    while (1) {
        halt_cpu();
    }
}

// Task execution monitoring interval (ticks)
#define PRINT_INTERVAL_TICKS 18  // ~180ms at 100Hz timer

// Busy-wait delay for task yield
#define TASK_YIELD_DELAY 100000

// Test task 1 - demonstrates multitasking
volatile uint32 task1_counter = 0;
void test_task_1(void) {
    print("[Task 1 started]\n");
    while (1) {
        task1_counter++;
        // Let other tasks run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Test task 2 - demonstrates multitasking
volatile uint32 task2_counter = 0;
void test_task_2(void) {
    print("[Task 2 started]\n");
    while (1) {
        task2_counter++;
        // Let other tasks run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Main kernel task - handles keyboard input and status display
void kernel_main_task(void) {
    print("Multitasking initialized. Tasks are running.\n");
    print("Task counters will increment in background.\n");
    print("Monitoring task execution...\n\n");
    
    uint32 last_print_tick = 0;
    while (1) {
        // Print task status every PRINT_INTERVAL_TICKS
        if (g_timer_ticks - last_print_tick > PRINT_INTERVAL_TICKS) {
            print("T1:");
            print_hex(task1_counter);
            print(" T2:");
            print_hex(task2_counter);
            print(" Ticks:");
            print_hex((uint32)g_timer_ticks);
            print("\n");
            last_print_tick = g_timer_ticks;
        }
        
        // Use hlt instruction to save power when idle
        halt_cpu();
    }
}

void kmain(uint32 magic, multiboot_info_t *mbi) {
    gdt_init();
    idt_init();
    
    // Register timer interrupt handler (IRQ0 = interrupt 32)
    isr_register_interrupt_handler(IRQ_BASE + IRQ0_TIMER, timer_interrupt_handler);
    
    // Don't enable interrupts yet - wait until ready
    
    // Verify multiboot magic
    if (magic != MULTIBOOT_MAGIC) {
        kernel_panic("Invalid multiboot magic number");
    }
    
    memory_init(mbi);
    paging_init();
    paging_enable();
    
    keyboard_init();
    // mouse_init();
    clear_screen();
    print("Welcome to AutismOS!\n");
    print("===================\n\n");
    beep();
    
    // Initialize syscall subsystem
    syscall_init();
    
    // Initialize user mode subsystem
    usermode_init();
    
    // Set up TSS for privilege level changes
    // We'll use a simple kernel stack
    uint32 kernel_stack;
    asm volatile("mov %%esp, %0" : "=r"(kernel_stack));
    tss_init(0x10, kernel_stack);
    print("TSS initialized\n");
    
    print("\n=== Step 3: User Mode & System Calls ===\n");
    print("Testing privilege separation (Ring 0 -> Ring 3)\n\n");
    
    // Allocate user space for the program
    uint32 prog_size = user_program_size;
    if (prog_size == 0 || prog_size > 4096) {
        prog_size = 4096;  // Fallback size
    }
    void* user_code = allocate_user_memory(prog_size);
    print("Allocated user code at: 0x");
    print_hex((uint32)user_code);
    print("\n");
    
    // Copy user program to user space
    memcpy(user_code, user_program_code, prog_size);
    print("User program copied to user space (");
    print_hex(prog_size);
    print(" bytes)\n");
    
    // Allocate user stack
    void* user_stack_bottom = allocate_user_memory(4096);  // 4KB stack
    uint32 user_stack_top = (uint32)user_stack_bottom + 4096;
    print("User stack allocated: 0x");
    print_hex((uint32)user_stack_bottom);
    print(" - 0x");
    print_hex(user_stack_top);
    print("\n\n");
    
    // Enable interrupts before switching to user mode
    asm volatile("sti");
    
    // Switch to user mode and execute user program
    // This should never return
    switch_to_user_mode((uint32)user_code, user_stack_top);
    
    // Should never reach here
    kernel_panic("Returned from user mode");
}

