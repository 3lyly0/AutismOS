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
#include "process.h"
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

// Test process 1 - demonstrates process isolation
volatile uint32 process1_counter = 0;
void test_process_1(void) {
    print("[Process 1 started]\n");
    while (1) {
        process1_counter++;
        // Let other processes run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Test process 2 - demonstrates process isolation
volatile uint32 process2_counter = 0;
void test_process_2(void) {
    print("[Process 2 started]\n");
    while (1) {
        process2_counter++;
        // Let other processes run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Main kernel process - handles monitoring
void kernel_main_process(void) {
    print("Process isolation initialized. Processes are running.\n");
    print("Process counters will increment in background.\n");
    print("Monitoring process execution...\n\n");
    
    uint32 last_print_tick = 0;
    while (1) {
        // Print process status every PRINT_INTERVAL_TICKS
        if (g_timer_ticks - last_print_tick > PRINT_INTERVAL_TICKS) {
            print("P1:");
            print_hex(process1_counter);
            print(" P2:");
            print_hex(process2_counter);
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
    
    print("\n=== Step 4: Processes & Address Spaces ===\n");
    print("Testing process isolation with separate address spaces\n\n");
    
    // Initialize task subsystem first
    task_init();
    
    // Create test tasks (not processes yet - just tasks for now)
    print("Creating kernel tasks...\n");
    task_t* task1 = task_create(test_process_1);
    print("Created task 1: ID=");
    print_hex(task1->id);
    print("\n");
    
    task_t* task2 = task_create(test_process_2);
    print("Created task 2: ID=");
    print_hex(task2->id);
    print("\n");
    
    task_t* main_task = task_create(kernel_main_process);
    print("Created main task: ID=");
    print_hex(main_task->id);
    print("\n\n");
    
    // Initialize process subsystem
    process_init();
    print("Process subsystem initialized (processes will be added in future)\n\n");
    
    // Enable interrupts to start scheduling
    print("Enabling interrupts and starting task scheduling...\n\n");
    asm volatile("sti");
    
    // Infinite loop - should never reach here as scheduler takes over
    while (1) {
        halt_cpu();
    }
}

