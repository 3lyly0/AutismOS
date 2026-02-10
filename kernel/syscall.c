#include "syscall.h"
#include "video.h"
#include "idt.h"
#include "isr.h"
#include "usermode.h"
#include "process.h"
#include "ipc.h"
#include "task.h"

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
        
        case SYS_SEND: {
            // SYS_SEND: Send IPC message to another process
            // EBX = target PID
            // ECX = pointer to message_t structure
            uint32 target_pid = regs->ebx;
            message_t* msg = (message_t*)regs->ecx;
            
            // Find target process
            process_t* target = process_find_by_pid(target_pid);
            if (!target) {
                regs->eax = -1;  // Process not found
                break;
            }
            
            // Set sender PID
            msg->sender_pid = current ? current->pid : 0;
            
            // Enqueue message to target's inbox
            int result = message_queue_enqueue(&target->inbox, msg);
            
            // Wake up target if it's waiting for messages
            if (result == 0 && target->main_thread && 
                target->main_thread->state == TASK_WAITING) {
                target->main_thread->state = TASK_READY;
            }
            
            regs->eax = result;  // 0 on success, -1 if queue full
            break;
        }
        
        case SYS_RECV: {
            // SYS_RECV: Receive IPC message (blocking)
            // EBX = pointer to message_t structure to fill
            message_t* msg = (message_t*)regs->ebx;
            
            if (!current) {
                regs->eax = -1;
                break;
            }
            
            // Try to dequeue a message
            int result = message_queue_dequeue(&current->inbox, msg);
            
            if (result == 0) {
                // Message received
                regs->eax = 0;
            } else {
                // No message available - block the task
                if (current->main_thread) {
                    current->main_thread->state = TASK_WAITING;
                }
                regs->eax = -1;  // Will be retried when task wakes
            }
            break;
        }
        
        case SYS_POLL: {
            // SYS_POLL: Check for IPC message (non-blocking)
            // EBX = pointer to message_t structure to fill
            message_t* msg = (message_t*)regs->ebx;
            
            if (!current) {
                regs->eax = -1;
                break;
            }
            
            // Try to dequeue a message (non-blocking)
            int result = message_queue_dequeue(&current->inbox, msg);
            regs->eax = result;  // 0 if message received, -1 if no message
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
