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
    }
    pic8259_eoi(reg->int_no);
}


void isr_exception_handler(REGISTERS reg) {
    if (reg.int_no < 32) {
        // Print exception information
        print("\n\nEXCEPTION: ");
        print(exception_messages[reg.int_no]);
        print("\nInterrupt number: ");
        print_hex(reg.int_no);
        print("\nError code: ");
        print_hex(reg.err_code);
        print("\nEIP: ");
        print_hex(reg.eip);
        print("\n");
        
        // Call kernel panic
        kernel_panic("Unhandled CPU Exception");
    }
    if (g_interrupt_handlers[reg.int_no] != NULL) {
        ISR handler = g_interrupt_handlers[reg.int_no];
        handler(&reg);
    }
}