#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "isr.h"

// Syscall numbers
#define SYS_WRITE 1

// Syscall handler
void syscall_handler(REGISTERS *regs);
void syscall_init(void);

#endif
