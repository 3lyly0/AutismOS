#include <stddef.h>
#include "isr.h"
#include "idt.h"
#include "8259_pic.h"
#include "video.h"
#include "kernel.h"


ISR g_interrupt_handlers[NO_INTERRUPT_HANDLERS];


char *exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available (No Math Coprocessor)",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection",
    "Page Fault",
    "Unknown Interrupt (intel reserved)",
    "x87 FPU Floating-Point Error (Math Fault)",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};


void isr_register_interrupt_handler(int num, ISR handler) {
    if (num < NO_INTERRUPT_HANDLERS)
        g_interrupt_handlers[num] = handler;
}

void isr_end_interrupt(int num) {
    pic8259_eoi(num);
}


void isr_irq_handler(REGISTERS *reg) {
    if (g_interrupt_handlers[reg->int_no] != NULL) {
        ISR handler = g_interrupt_handlers[reg->int_no];
        handler(reg);
    } else {
        // Log unhandled interrupt for debugging (with basic rate limiting)
        // Only log first occurrence of each unhandled interrupt type
        static uint8 logged_interrupts[NO_INTERRUPT_HANDLERS] = {0};
        if (!logged_interrupts[reg->int_no]) {
            print("WARNING: Unhandled interrupt ");
            print_hex(reg->int_no);
            print("\n");
            logged_interrupts[reg->int_no] = 1;
        }
    }
    pic8259_eoi(reg->int_no);
}


// Page fault handler - reads CR2 to get the faulting address
void page_fault_handler(REGISTERS *reg) {
    uint32 faulting_address;
    
    // Read CR2 to get the address that caused the fault
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    print("\n\nPAGE FAULT\n");
    print("Address: ");
    print_hex(faulting_address);
    print("\n");
    print("Error code: ");
    print_hex(reg->err_code);
    print("\n");
    
    // Decode error code
    print("Details: ");
    if (reg->err_code & 0x1) {
        print("Protection violation ");
    } else {
        print("Not-present ");
    }
    
    if (reg->err_code & 0x2) {
        print("Write ");
    } else {
        print("Read ");
    }
    
    if (reg->err_code & 0x4) {
        print("User-mode\n");
    } else {
        print("Kernel-mode\n");
    }
    
    print("EIP: ");
    print_hex(reg->eip);
    print("\n");
    
    kernel_panic("Page Fault Exception");
}

void isr_exception_handler(REGISTERS reg) {
    // Special handling for page faults (exception 14)
    if (reg.int_no == 14) {
        page_fault_handler(&reg);
        return;
    }
    
    if (reg.int_no < 32) {
        print("\n\nEXCEPTION: ");
        print(exception_messages[reg.int_no]);
        print("\nInterrupt number: ");
        print_hex(reg.int_no);
        print("\nError code: ");
        print_hex(reg.err_code);
        print("\nEIP: ");
        print_hex(reg.eip);
        print("\n");
        
        kernel_panic("Unhandled CPU Exception");
    }
    if (g_interrupt_handlers[reg.int_no] != NULL) {
        ISR handler = g_interrupt_handlers[reg.int_no];
        handler(&reg);
    }
}