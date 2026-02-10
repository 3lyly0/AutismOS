#ifndef USERMODE_H
#define USERMODE_H

#include "types.h"

// User space memory region
#define USER_SPACE_START 0x00200000  // 2MB (after kernel)
#define USER_SPACE_END   0x003FFFFF  // Just before 4MB

// Initialize user mode subsystem
void usermode_init(void);

// Allocate user space memory
void* allocate_user_memory(uint32 size);

// Switch to user mode and execute code at given address
void switch_to_user_mode(uint32 entry_point, uint32 user_stack);

#endif
