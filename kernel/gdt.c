#include "gdt.h"
#include "string.h"

GDT g_gdt[NO_GDT_DESCRIPTORS];
GDT_PTR g_gdt_ptr;
TSS g_tss;

void gdt_set_entry(int index, uint32 base, uint32 limit, uint8 access, uint8 gran) {
    GDT *this = &g_gdt[index];

    this->segment_limit = limit & 0xFFFF;
    this->base_low = base & 0xFFFF;
    this->base_middle = (base >> 16) & 0xFF;
    this->access = access;

    this->granularity = (limit >> 16) & 0x0F;
    this->granularity = this->granularity | (gran & 0xF0);

    this->base_high = (base >> 24 & 0xFF);
}


void gdt_init() {
    g_gdt_ptr.limit = sizeof(g_gdt) - 1;
    g_gdt_ptr.base_address = (uint32)g_gdt;

    gdt_set_entry(0, 0, 0, 0, 0);               // Null descriptor
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);  // Kernel code
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);  // Kernel data
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);  // User code
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);  // User data
    // Entry 5 will be TSS

    load_gdt((uint32)&g_gdt_ptr);
}

void tss_init(uint32 kernel_ss, uint32 kernel_esp) {
    // Clear TSS
    memset(&g_tss, 0, sizeof(TSS));
    
    // Set up TSS
    g_tss.ss0 = kernel_ss;   // Kernel data segment (0x10)
    g_tss.esp0 = kernel_esp; // Kernel stack pointer
    g_tss.cs = 0x08;         // Kernel code segment (RPL=0)
    g_tss.ss = 0x10;         // Kernel data segment (RPL=0)
    g_tss.ds = 0x10;
    g_tss.es = 0x10;
    g_tss.fs = 0x10;
    g_tss.gs = 0x10;
    
    // Add TSS descriptor to GDT
    // Access: 0xE9 = Present, Executable, Accessed, 32-bit TSS
    uint32 base = (uint32)&g_tss;
    uint32 limit = sizeof(TSS);
    gdt_set_entry(5, base, limit, 0x89, 0x00);
    
    // Load TSS
    tss_flush();
}
