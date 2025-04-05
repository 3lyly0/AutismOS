#ifndef IDT_H
#define IDT_H

#include "types.h"

#define NO_IDT_DESCRIPTORS     256

typedef struct {
    uint16 base_low;
    uint16 segment_selector;
    uint8 zero;
    uint8 type;
    uint16 base_high;
} __attribute__((packed)) IDT;

typedef struct {
    uint16 limit;
    uint32 base_address;
} __attribute__((packed)) IDT_PTR;


extern void load_idt(uint32 idt_ptr);
void idt_set_entry(int index, uint32 base, uint16 seg_sel, uint8 flags);
void idt_init();

#endif
