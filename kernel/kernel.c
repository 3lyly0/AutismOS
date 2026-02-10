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
#include "ipc.h"

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

// Message types for IPC demonstration
#define MSG_TYPE_REQUEST    1
#define MSG_TYPE_RESPONSE   2
#define MSG_TYPE_RENDER     3
#define MSG_TYPE_FRAME      4

// Inline syscall wrappers for kernel mode (for demonstration)
static inline int sys_send_msg(uint32 target_pid, uint32 type, uint32 data1, uint32 data2) {
    message_t msg;
    msg.sender_pid = 0;  // Will be set by syscall handler
    msg.type = type;
    msg.data1 = data1;
    msg.data2 = data2;
    
    int result;
    asm volatile("int $0x80" 
                 : "=a"(result) 
                 : "a"(SYS_SEND), "b"(target_pid), "c"(&msg));
    return result;
}

static inline int sys_poll_msg(message_t* msg) {
    int result;
    asm volatile("int $0x80" 
                 : "=a"(result) 
                 : "a"(SYS_POLL), "b"(msg));
    return result;
}

// Browser process - simulates browser control process
volatile uint32 browser_counter = 0;
void browser_process(void) {
    print("[Browser Process] Started (simulates UI/control process)\n");
    
    uint32 renderer_pid = 0;  // Renderer was created first (PID 0)
    uint32 frame_count = 0;
    
    while (1) {
        browser_counter++;
        
        // Every so often, send render request to renderer
        if (browser_counter % 5 == 0) {
            sys_send_msg(renderer_pid, MSG_TYPE_RENDER, frame_count++, 0);
        }
        
        // Poll for responses from renderer
        message_t msg;
        if (sys_poll_msg(&msg) == 0) {
            if (msg.type == MSG_TYPE_FRAME) {
                // Received frame ready notification
                // (In real browser, would composite to screen)
            }
        }
        
        // Let other processes run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Renderer process - simulates HTML/layout renderer
volatile uint32 renderer_counter = 0;
void renderer_process(void) {
    print("[Renderer Process] Started (simulates HTML/layout engine)\n");
    
    while (1) {
        renderer_counter++;
        
        // Poll for render requests from browser
        message_t msg;
        if (sys_poll_msg(&msg) == 0) {
            if (msg.type == MSG_TYPE_RENDER) {
                // Received render request
                // Do "rendering" work (just increment counter)
                
                // Send frame ready back to browser
                sys_send_msg(msg.sender_pid, MSG_TYPE_FRAME, msg.data1, 0);
            }
        }
        
        // Let other processes run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Main kernel process - handles monitoring
void kernel_main_process(void) {
    print("IPC and messaging initialized. Processes communicating.\n");
    print("Browser <-> Renderer IPC demonstration running.\n");
    print("Monitoring process execution and message passing...\n\n");
    
    uint32 last_print_tick = 0;
    while (1) {
        // Print process status every PRINT_INTERVAL_TICKS
        if (g_timer_ticks - last_print_tick > PRINT_INTERVAL_TICKS) {
            print("Browser:");
            print_hex(browser_counter);
            print(" Renderer:");
            print_hex(renderer_counter);
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
    
    print("\n=== Step 5: IPC, Messaging & Event-Driven Execution ===\n");
    print("Demonstrating IPC between browser and renderer processes\n\n");
    
    // Initialize task subsystem first
    task_init();
    
    // Initialize process subsystem
    process_init();
    
    // Create processes with IPC capabilities
    print("Creating processes with message queues...\n");
    process_t* renderer_proc = process_create(renderer_process, 0);
    print("  -> Renderer process: PID=");
    print_hex(renderer_proc->pid);
    print(" (HTML/Layout engine)\n");
    
    process_t* browser_proc = process_create(browser_process, 0);
    print("  -> Browser process: PID=");
    print_hex(browser_proc->pid);
    print(" (UI/Control)\n");
    
    process_t* main_proc = process_create(kernel_main_process, 0);
    print("  -> Monitor process: PID=");
    print_hex(main_proc->pid);
    print(" (System monitor)\n");
    
    print("\nNote: Each process has its own message queue.\n");
    print("Processes communicate via kernel-mediated IPC.\n");
    print("Browser sends RENDER requests to Renderer.\n");
    print("Renderer sends FRAME_READY responses back.\n\n");
    
    // Enable interrupts to start scheduling
    print("Enabling interrupts and starting process scheduling...\n\n");
    asm volatile("sti");
    
    // Main kernel loop - the scheduler will interrupt and switch to tasks
    // This acts as an idle loop
    while (1) {
        halt_cpu();
    }
}

