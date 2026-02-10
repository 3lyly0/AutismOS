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

// Test task 1 - demonstrates multitasking
volatile uint32 task1_counter = 0;
void test_task_1(void) {
    print("[Task 1 started]\n");
    while (1) {
        task1_counter++;
        // Let other tasks run
        for (volatile int i = 0; i < 100000; i++);
    }
}

// Test task 2 - demonstrates multitasking
volatile uint32 task2_counter = 0;
void test_task_2(void) {
    print("[Task 2 started]\n");
    while (1) {
        task2_counter++;
        // Let other tasks run
        for (volatile int i = 0; i < 100000; i++);
    }
}

// Main kernel task - handles keyboard input and status display
void kernel_main_task(void) {
    print("Multitasking initialized. Tasks are running.\n");
    print("Task counters will increment in background.\n");
    print("Monitoring task execution...\n\n");
    
    uint32 last_print_tick = 0;
    while (1) {
        // Print task status every ~18 ticks (roughly every 180ms at 100Hz)
        if (g_timer_ticks - last_print_tick > 18) {
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
    
    // Don't enable interrupts yet - wait until tasks are set up
    
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
    beep();
    
    // Initialize task system
    print("Initializing multitasking...\n");
    task_init();
    
    // Create kernel tasks
    print("Creating idle task...\n");
    task_create(idle_task);
    
    print("Creating test task 1...\n");
    task_create(test_task_1);
    
    print("Creating test task 2...\n");
    task_create(test_task_2);
    
    print("Creating kernel main task...\n");
    task_create(kernel_main_task);
    
    print("Starting multitasking...\n\n");
    
    // Now enable interrupts and enter idle loop
    // The scheduler will switch to the tasks we created
    asm volatile("sti");
    
    // Main kernel thread becomes the idle loop
    while (1) {
        halt_cpu();
    }
}

