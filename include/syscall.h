#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "isr.h"

// Syscall numbers
#define SYS_WRITE      1
#define SYS_SEND       2  // Send IPC message (Step 5)
#define SYS_RECV       3  // Receive IPC message - blocking (Step 5)
#define SYS_POLL       4  // Poll for IPC message - non-blocking (Step 5)
#define SYS_SHM_CREATE 5  // Create shared memory region (Step 6)
#define SYS_SHM_MAP    6  // Map shared memory region (Step 6)
#define SYS_SHM_UNMAP  7  // Unmap shared memory region (Step 6)

// Syscall handler
void syscall_handler(REGISTERS *regs);
void syscall_init(void);

#endif
