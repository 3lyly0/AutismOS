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

// Task State Segment structure
typedef struct {
    uint32 prev_tss;
    uint32 esp0;     // Kernel stack pointer
    uint32 ss0;      // Kernel stack segment
    uint32 esp1;
    uint32 ss1;
    uint32 esp2;
    uint32 ss2;
    uint32 cr3;
    uint32 eip;
    uint32 eflags;
    uint32 eax;
    uint32 ecx;
    uint32 edx;
    uint32 ebx;
    uint32 esp;
    uint32 ebp;
    uint32 esi;
    uint32 edi;
    uint32 es;
    uint32 cs;
    uint32 ss;
    uint32 ds;
    uint32 fs;
    uint32 gs;
    uint32 ldt;
    uint16 trap;
    uint16 iomap_base;
} __attribute__((packed)) TSS;

extern void load_gdt(uint32 gdt_ptr);
extern void tss_flush(void);
void gdt_set_entry(int index, uint32 base, uint32 limit, uint8 access, uint8 gran);
void gdt_init();
void tss_init(uint32 kernel_ss, uint32 kernel_esp);

#endif
