#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

extern uint8 __kernel_section_start;
extern uint8 __kernel_section_end;
extern uint8 __kernel_text_section_start;
extern uint8 __kernel_text_section_end;
extern uint8 __kernel_data_section_start;
extern uint8 __kernel_data_section_end;
extern uint8 __kernel_rodata_section_start;
extern uint8 __kernel_rodata_section_end;
extern uint8 __kernel_bss_section_start;
extern uint8 __kernel_bss_section_end;

// Kernel panic handler - never returns
void kernel_panic(const char* message);

// Global timer tick counter
extern volatile uint64 g_timer_ticks;

#endif
