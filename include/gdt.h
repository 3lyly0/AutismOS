#ifndef GDT_H
#define GDT_H

#include "types.h"

#define NO_GDT_DESCRIPTORS     8

typedef struct {
    uint16 segment_limit;
    uint16 base_low;
    uint8 base_middle;
    uint8 access;
    uint8 granularity;
    uint8 base_high;
} __attribute__((packed)) GDT;

typedef struct {
    uint16 limit;
    uint32 base_address;
} __attribute__((packed)) GDT_PTR;


extern void load_gdt(uint32 gdt_ptr);
void gdt_set_entry(int index, uint32 base, uint32 limit, uint8 access, uint8 gran);
void gdt_init();

#endif
