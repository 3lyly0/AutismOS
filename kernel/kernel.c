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

// Global tick counter for timer
volatile uint64 g_timer_ticks = 0;

// Timer interrupt handler
void timer_interrupt_handler(REGISTERS *regs) {
    (void)regs; // Suppress unused parameter warning
    g_timer_ticks++;
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

void kmain(uint32 magic, multiboot_info_t *mbi) {
    gdt_init();
    idt_init();
    
    // Register timer interrupt handler (IRQ0 = interrupt 32)
    isr_register_interrupt_handler(IRQ_BASE + IRQ0_TIMER, timer_interrupt_handler);
    
    // Now it's safe to enable interrupts
    asm volatile("sti");
    
    // Verify multiboot magic
    if (magic != MULTIBOOT_MAGIC) {
        kernel_panic("Invalid multiboot magic number");
    }
    
    memory_init(mbi);
    paging_init();
    paging_enable();
    
    // Test kmalloc
    print("\nTesting kmalloc...\n");
    void *ptr1 = kmalloc(64);
    print("Allocated 64 bytes at: 0x");
    print_hex((uint32_t)ptr1);
    print("\n");
    
    void *ptr2 = kmalloc(128);
    print("Allocated 128 bytes at: 0x");
    print_hex((uint32_t)ptr2);
    print("\n");
    
    void *ptr3 = kmalloc(256);
    print("Allocated 256 bytes at: 0x");
    print_hex((uint32_t)ptr3);
    print("\n");
    
    print("kmalloc test passed!\n\n");
    
    keyboard_init();
    // mouse_init();
    clear_screen();
    print("Welcome to AutismOS!\n");
    beep();
    print("Press any key to see it on the screen...\n");



    while (1) {
        char c = kb_getchar();
        if (c != 0) {
            char str[2] = {c, '\0'};
            print(str);
        }
        // Use hlt instruction to save power when idle
        halt_cpu();
    }
}

