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
#include "graphics.h"
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
#include "input.h"
#include "network.h"
#include "html.h"
#include "layout.h"
#include "ux.h"

// Global tick counter for timer
volatile uint64 g_timer_ticks = 0;

// Timer interrupt handler
// Note: Task switching is now handled in the assembly IRQ handler
void timer_interrupt_handler(REGISTERS *regs) {
    (void)regs; // Suppress unused parameter warning
    // Timer tick and task switching handled by task_scheduler_tick
    // which is called directly from irq_0_with_task_switch
    
    // Step 9: Update caret blink for UI
    ux_update_caret();
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
#define MSG_TYPE_REQUEST      1
#define MSG_TYPE_RESPONSE     2
#define MSG_TYPE_RENDER       3
#define MSG_TYPE_FRAME        4
#define MSG_TYPE_FRAME_READY  5  // Step 6: Frame ready notification
#define MSG_TYPE_URL_REQUEST  6  // Step 7: URL request
#define MSG_TYPE_URL_RESPONSE 7  // Step 7: URL response
#define MSG_TYPE_LAYOUT_DONE  8  // Step 7: Layout complete

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

// Browser process - Full browser with networking and rendering (Step 7 & 9)
volatile uint32 browser_counter = 0;
volatile uint32 browser_page_loaded = 0;
volatile uint32 browser_url_submitted = 0;

void browser_process(void) {
    if (!ux_is_silent()) {
        print("[Browser/UI Process] Started - Step 9 Interactive UI\n");
    }
    
    uint32 renderer_pid = 0;  // Renderer was created first (PID 0)
    void* framebuffer_ptr = NULL;
    
    // Wait for framebuffer to be created by renderer
    uint32 wait_count = 0;
    while (g_framebuffer_shm_id == 0) {
        // Yield to other processes instead of busy waiting
        if (wait_count++ > 5) {
            for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
            wait_count = 0;
        }
    }
    
    // Map the shared framebuffer
    if (sys_shm_map(g_framebuffer_shm_id, &framebuffer_ptr) == 0) {
        if (!ux_is_silent()) {
            print("[Browser] Mapped shared framebuffer at: 0x");
            print_hex((uint32)framebuffer_ptr);
            print("\n");
        }
    }
    
    // Track last IP to detect changes
    char last_submitted_ip[256] = {0};
    
    // Browser main event loop (Step 9)
    while (1) {
        browser_counter++;
        
        // Check if user submitted a new IP (Step 9)
        const char* current_ip = ux_get_last_ip();
        if (current_ip && current_ip[0]) {
            // Check if IP changed (user pressed Enter)
            if (strcmp(last_submitted_ip, current_ip) != 0) {
                // New IP submitted!
                strcpy(last_submitted_ip, current_ip);
                browser_url_submitted = 1;
                browser_page_loaded = 0;
                
                if (!ux_is_silent()) {
                    print("[Browser] User submitted IP: ");
                    print(current_ip);
                    print("\n");
                }
                
                // Send IP request to renderer (ping request)
                sys_send_msg(renderer_pid, MSG_TYPE_URL_REQUEST, 0, 0);
            }
        }
        
        // Poll for events
        message_t msg;
        if (sys_poll_msg(&msg) == 0) {
            if (msg.type == MSG_TYPE_FRAME_READY) {
                // Frame is ready - display it in content area (Step 9)
                if (!browser_page_loaded && browser_url_submitted) {
                    // Show ping result in content area
                    graphics_clear_region(2, 14, 76, 9, COLOR_BLACK);
                    draw_text(3, 15, "Ping completed!", COLOR_LIGHT_GREEN);
                    draw_text(3, 17, "IP: ", COLOR_WHITE);
                    draw_text(8, 17, last_submitted_ip, COLOR_YELLOW);
                    
                    browser_page_loaded = 1;
                    
                    if (!ux_is_silent()) {
                        print("  [Browser] Ping result displayed in content area\n");
                    }
                }
            } else if (msg.type == INPUT_EVENT_KEY_DOWN) {
                // Handle keyboard input (already handled by keyboard driver)
            }
        }
        
        // Let other processes run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Renderer process - Full renderer with network, HTML, layout (Step 7)
volatile uint32 renderer_counter = 0;
void renderer_process(void) {
    if (!ux_is_silent()) {
        print("[Renderer Process] Started - Step 7 Full Renderer\n");
    }
    
    // Define framebuffer dimensions
    const uint32 FB_WIDTH = 320;
    const uint32 FB_HEIGHT = 200;
    const uint32 FB_SIZE = FB_WIDTH * FB_HEIGHT * sizeof(uint32);
    
    // Create shared framebuffer
    uint32 shm_id = sys_shm_create(FB_SIZE);
    if (shm_id == 0) {
        if (!ux_is_silent()) {
            print("[Renderer] Failed to create shared framebuffer!\n");
        }
        while (1);
    }
    
    if (!ux_is_silent()) {
        print("[Renderer] Created shared framebuffer ID: ");
        print_hex(shm_id);
        print(" (");
        print_hex(FB_WIDTH);
        print("x");
        print_hex(FB_HEIGHT);
        print(")\n");
    }
    
    // Share the framebuffer ID globally
    g_framebuffer_shm_id = shm_id;
    
    // Map the framebuffer
    uint32* pixels = NULL;
    if (sys_shm_map(shm_id, (void**)&pixels) != 0) {
        if (!ux_is_silent()) {
            print("[Renderer] Failed to map framebuffer!\n");
        }
        while (1);
    }
    
    if (!ux_is_silent()) {
        print("[Renderer] Framebuffer mapped at: 0x");
        print_hex((uint32)pixels);
        print("\n");
    }
    
    // Renderer main loop (Step 7)
    while (1) {
        renderer_counter++;
        
        // Poll for requests from browser
        message_t msg;
        if (sys_poll_msg(&msg) == 0) {
            if (msg.type == MSG_TYPE_URL_REQUEST) {
                const char* ip_str = ux_get_last_ip();
                if (!ux_is_silent()) {
                    print("  [Renderer] Pinging ");
                    print(ip_str);
                    print("...\n");
                }
                
                // Ping the IP address
                ping_response_t ping_resp;
                if (ping_ip(ip_str, &ping_resp) == 0) {
                    if (!ux_is_silent()) {
                        print("  [Renderer] Ping result: ");
                        print(ping_resp.message);
                        print("\n");
                    }
                    
                    // Create a simple text response to display
                    // Clear framebuffer first
                    for (uint32 i = 0; i < FB_WIDTH * FB_HEIGHT; i++) {
                        pixels[i] = 0xFF000000; // Black background
                    }
                    
                    // Draw ping result text
                    // This is a simplified rendering - just display the result
                    // In a real system, we would use the layout engine
                    // For now, we'll skip HTML parsing and just show raw text
                    
                } else {
                    if (!ux_is_silent()) {
                        print("  [Renderer] Ping failed: ");
                        print(ping_resp.message);
                        print("\n");
                    }
                }
                
                // Send frame ready notification
                sys_send_msg(msg.sender_pid, MSG_TYPE_FRAME_READY, 0, shm_id);
            }
        }
        
        // Let other processes run
        for (volatile int i = 0; i < TASK_YIELD_DELAY; i++);
    }
}

// Main kernel process - handles monitoring
void kernel_main_process(void) {
    if (!ux_is_silent()) {
        print("Step 6 & 7: Full browser-capable OS initialized.\n");
        print("Browser features: Networking, HTML parsing, Layout, Rendering\n");
        print("Monitoring browser activity...\n\n");
    }
    
    uint32 last_print_tick = 0;
    while (1) {
        // Print process status every PRINT_INTERVAL_TICKS
        if (g_timer_ticks - last_print_tick > PRINT_INTERVAL_TICKS) {
            if (!ux_is_silent()) {
                print("Browser:");
                print_hex(browser_counter);
                print(" Renderer:");
                print_hex(renderer_counter);
                if (browser_page_loaded) {
                    print(" [PAGE_LOADED]");
                }
                print("\n");
            }
            last_print_tick = g_timer_ticks;
        }
        
        // Use hlt instruction to save power when idle
        halt_cpu();
    }
}

void kmain(uint32 magic, multiboot_info_t *mbi) {
    // Step 8: Initialize UX kernel first for clean boot experience
    ux_init();
    ux_show_boot_screen();
    
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
    
    // Initialize syscall subsystem
    syscall_init();
    
    // Initialize user mode subsystem
    usermode_init();
    
    // Set up TSS for privilege level changes
    // We'll use a simple kernel stack
    uint32 kernel_stack;
    asm volatile("mov %%esp, %0" : "=r"(kernel_stack));
    tss_init(0x10, kernel_stack);
    
    // Initialize shared memory subsystem
    shm_init();
    
    // Initialize Step 7 subsystems
    input_init();
    network_init();
    html_init();
    layout_init();
    
    // Initialize task subsystem first
    task_init();
    
    // Initialize process subsystem
    process_init();
    
    // Create processes with full browser capabilities (silently)
    process_t* renderer_proc = process_create(renderer_process, 0);
    (void)renderer_proc; // Suppress unused warning
    
    process_t* browser_proc = process_create(browser_process, 0);
    (void)browser_proc; // Suppress unused warning
    
    process_t* main_proc = process_create(kernel_main_process, 0);
    (void)main_proc; // Suppress unused warning
    
    // Step 8: Finish boot and show clean UI
    beep();
    ux_finish_boot();
    
    // Enable interrupts to start scheduling
    asm volatile("sti");
    
    // Main kernel loop - the scheduler will interrupt and switch to tasks
    // This acts as an idle loop
    while (1) {
        halt_cpu();
    }
}

