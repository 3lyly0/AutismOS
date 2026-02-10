#ifndef TASK_H
#define TASK_H

#include "types.h"

// Task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_WAITING    // Waiting for IPC message (Step 5)
} task_state_t;

// Minimum viable task structure
typedef struct task {
    uint32 esp;       // Stack pointer
    uint32 ebp;       // Base pointer
    uint32 eip;       // Instruction pointer
    uint32 id;        // Task ID
    task_state_t state;
    void* process;    // Pointer to parent process (opaque to avoid circular dependency)
    struct task* next; // Next task in circular list
} task_t;

// Task management functions
void task_init(void);
task_t* task_create(void (*entry_point)(void));
task_t* task_get_current(void);
uint32 task_scheduler_tick(uint32 current_esp);

#endif
