#include "isr.h"
#include "idt.h"
#include "io_ports.h"
#include "8259_pic.h"


void pic8259_init() {
    uint8 a1, a2;

    a1 = inportb(PIC1_DATA);
    a2 = inportb(PIC2_DATA);

    outportb(PIC1_COMMAND, ICW1);
    outportb(PIC2_COMMAND, ICW1);
    outportb(PIC1_DATA, 0x20);
    outportb(PIC2_DATA, 0x28);
    outportb(PIC1_DATA, 4);
    outportb(PIC2_DATA, 2);
    outportb(PIC1_DATA, ICW4_8086);
    outportb(PIC2_DATA, ICW4_8086);
    outportb(PIC1_DATA, a1);
    outportb(PIC2_DATA, a2);
}


void pic8259_eoi(uint8 irq) {
    if(irq >= 0x28)
        outportb(PIC2, PIC_EOI);
    outportb(PIC1, PIC_EOI);
}

