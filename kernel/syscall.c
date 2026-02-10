#include "syscall.h"
#include "video.h"
#include "idt.h"
#include "isr.h"
#include "usermode.h"
#include "process.h"

// Syscall handler - called when user mode executes int 0x80
void syscall_handler(REGISTERS *regs) {
    // Get current process for context
    process_t* current = process_get_current();
    
    // Get syscall number from EAX
    uint32 syscall_num = regs->eax;
    
    switch (syscall_num) {
        case SYS_WRITE: {
            // SYS_WRITE: Print a string
            // EBX = pointer to string
            char *str = (char *)regs->ebx;
            
            // Validate pointer is in user space
            if ((uint32)str < USER_SPACE_START || (uint32)str >= USER_SPACE_END) {
                print("Syscall error: Invalid pointer (PID=");
                if (current) {
                    print_hex(current->pid);
                } else {
                    print("?");
                }
                print(")\n");
                regs->eax = -1;
                return;
            }
            
            // Additional safety: limit string length to prevent reading past user space
            uint32 max_len = USER_SPACE_END - (uint32)str;
            if (max_len > 1024) max_len = 1024;  // Reasonable limit
            
            // Print with length check
            for (uint32 i = 0; i < max_len && str[i] != '\0'; i++) {
                char c[2] = {str[i], '\0'};
                print(c);
            }
            
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
