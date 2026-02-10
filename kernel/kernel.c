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
#include "shm.h"

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
#define MSG_TYPE_FRAME_READY 5  // Step 6: Frame ready notification

// Shared framebuffer ID (global)
volatile uint32 g_framebuffer_shm_id = 0;

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

static inline uint32 sys_shm_create(uint32 size) {
    uint32 result;
    asm volatile("int $0x80" 
                 : "=a"(result) 
                 : "a"(SYS_SHM_CREATE), "b"(size));
    return result;
}

static inline int sys_shm_map(uint32 shm_id, void** vaddr_out) {
    int result;
    asm volatile("int $0x80" 
                 : "=a"(result) 
                 : "a"(SYS_SHM_MAP), "b"(shm_id), "c"(vaddr_out));
    return result;
}

// Browser process - simulates browser control process (Step 6 enhanced)
volatile uint32 browser_counter = 0;
void browser_process(void) {
    print("[Browser/UI Process] Started - Step 6\n");
    
    uint32 renderer_pid = 0;  // Renderer was created first (PID 0)
    uint32 frame_count = 0;
    void* framebuffer_ptr = NULL;
    
    // Wait for framebuffer to be created by renderer
    while (g_framebuffer_shm_id == 0) {
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
    
    // Map the shared framebuffer
    if (sys_shm_map(g_framebuffer_shm_id, &framebuffer_ptr) == 0) {
        print("[Browser] Mapped shared framebuffer at: 0x");
        print_hex((uint32)framebuffer_ptr);
        print("\n");
    }
    
    while (1) {
        browser_counter++;
        
        // Every so often, send render request to renderer
        if (browser_counter % 5 == 0) {
            sys_send_msg(renderer_pid, MSG_TYPE_RENDER, frame_count++, 0);
        }
        
        // Poll for responses from renderer
        message_t msg;
        if (sys_poll_msg(&msg) == 0) {
            if (msg.type == MSG_TYPE_FRAME_READY) {
                // Received frame ready notification
                // In real browser, would composite framebuffer to screen
                // For now, just acknowledge receipt
            }
        }
        
        // Let other processes run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Renderer process - simulates HTML/layout renderer (Step 6 enhanced)
volatile uint32 renderer_counter = 0;
void renderer_process(void) {
    print("[Renderer Process] Started - Step 6 with shared framebuffer\n");
    
    // Define framebuffer dimensions
    const uint32 FB_WIDTH = 320;
    const uint32 FB_HEIGHT = 200;
    const uint32 FB_SIZE = FB_WIDTH * FB_HEIGHT * sizeof(uint32);
    
    // Create shared framebuffer
    uint32 shm_id = sys_shm_create(FB_SIZE);
    if (shm_id == 0) {
        print("[Renderer] Failed to create shared framebuffer!\n");
        while (1);
    }
    
    print("[Renderer] Created shared framebuffer ID: ");
    print_hex(shm_id);
    print(" (");
    print_hex(FB_WIDTH);
    print("x");
    print_hex(FB_HEIGHT);
    print(")\n");
    
    // Share the framebuffer ID globally
    g_framebuffer_shm_id = shm_id;
    
    // Map the framebuffer
    uint32* pixels = NULL;
    if (sys_shm_map(shm_id, (void**)&pixels) != 0) {
        print("[Renderer] Failed to map framebuffer!\n");
        while (1);
    }
    
    print("[Renderer] Framebuffer mapped at: 0x");
    print_hex((uint32)pixels);
    print("\n");
    
    uint32 color_index = 0;
    
    while (1) {
        renderer_counter++;
        
        // Poll for render requests from browser
        message_t msg;
        if (sys_poll_msg(&msg) == 0) {
            if (msg.type == MSG_TYPE_RENDER) {
                // Received render request
                // "Render" by filling framebuffer with a pattern
                // Cycle through colors to show it's working
                uint32 colors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF, 0x00FFFF};
                uint32 color = colors[color_index % 6];
                color_index++;
                
                // Fill a small rectangle to show activity
                for (uint32 y = 10; y < 30; y++) {
                    for (uint32 x = 10; x < 50; x++) {
                        if (x < FB_WIDTH && y < FB_HEIGHT) {
                            pixels[y * FB_WIDTH + x] = color;
                        }
                    }
                }
                
                // Send frame ready notification back to browser
                sys_send_msg(msg.sender_pid, MSG_TYPE_FRAME_READY, msg.data1, shm_id);
            }
        }
        
        // Let other processes run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Main kernel process - handles monitoring
void kernel_main_process(void) {
    print("Step 6: Shared memory and framebuffer rendering initialized.\n");
    print("Browser <-> Renderer communication with zero-copy framebuffer.\n");
    print("Monitoring process execution and frame rendering...\n\n");
    
    uint32 last_print_tick = 0;
    while (1) {
        // Print process status every PRINT_INTERVAL_TICKS
        if (g_timer_ticks - last_print_tick > PRINT_INTERVAL_TICKS) {
            print("Browser:");
            print_hex(browser_counter);
            print(" Renderer:");
            print_hex(renderer_counter);
            print(" FB_ID:");
            print_hex(g_framebuffer_shm_id);
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
    
    print("\n=== Step 6: Shared Memory, Graphics & Rendering ===\n");
    print("Demonstrating zero-copy framebuffer rendering\n\n");
    
    // Initialize shared memory subsystem
    shm_init();
    print("Shared memory subsystem initialized\n");
    
    // Initialize task subsystem first
    task_init();
    
    // Initialize process subsystem
    process_init();
    
    // Create processes with IPC and shared memory capabilities
    print("Creating processes with IPC and shared memory...\n");
    process_t* renderer_proc = process_create(renderer_process, 0);
    print("  -> Renderer process: PID=");
    print_hex(renderer_proc->pid);
    print(" (HTML/Layout + Framebuffer)\n");
    
    process_t* browser_proc = process_create(browser_process, 0);
    print("  -> Browser process: PID=");
    print_hex(browser_proc->pid);
    print(" (UI/Compositor)\n");
    
    process_t* main_proc = process_create(kernel_main_process, 0);
    print("  -> Monitor process: PID=");
    print_hex(main_proc->pid);
    print(" (System monitor)\n");
    
    print("\nNote: Renderer creates shared framebuffer.\n");
    print("Browser maps the same framebuffer (zero-copy).\n");
    print("Renderer draws pixels, sends FRAME_READY event.\n");
    print("Browser receives event and displays frame.\n\n");
    
    // Enable interrupts to start scheduling
    print("Enabling interrupts and starting process scheduling...\n\n");
    asm volatile("sti");
    
    // Main kernel loop - the scheduler will interrupt and switch to tasks
    // This acts as an idle loop
    while (1) {
        halt_cpu();
    }
}

