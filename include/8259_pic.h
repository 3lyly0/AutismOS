#ifndef _8259_PIC_H
#define _8259_PIC_H

#include "types.h"

#define PIC1            0x20
#define PIC2            0xA0
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1+1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2+1)
#define PIC_EOI         0x20
#define ICW1            0x11
#define ICW4_8086       0x01


void pic8259_init();
void pic8259_eoi(uint8 irq);

#endif

