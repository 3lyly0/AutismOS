#include "syscall.h"
#include "video.h"
#include "idt.h"
#include "isr.h"

// Syscall handler - called when user mode executes int 0x80
void syscall_handler(REGISTERS *regs) {
    // Get syscall number from EAX
    uint32 syscall_num = regs->eax;
    
    switch (syscall_num) {
        case SYS_WRITE: {
            // SYS_WRITE: Print a string
            // EBX = pointer to string
            char *str = (char *)regs->ebx;
            print(str);
            regs->eax = 0; // Return success
            break;
        }
        default:
            // Unknown syscall
            print("Unknown syscall: ");
            print_hex(syscall_num);
            print("\n");
            regs->eax = -1; // Return error
            break;
    }
}

// Initialize syscall subsystem
void syscall_init(void) {
    // Register syscall handler for interrupt 0x80
    isr_register_interrupt_handler(0x80, syscall_handler);
    print("Syscall subsystem initialized (int 0x80)\n");
}
