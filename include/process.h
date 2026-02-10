#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "task.h"

// Forward declaration
typedef struct page_directory page_directory_t;

// Process structure - minimum viable process for Step 4
typedef struct process {
    uint32 pid;                      // Process ID
    page_directory_t* page_dir;      // Process page directory
    task_t* main_thread;             // Main thread (task) of this process
    struct process* next;            // Next process in circular list
} process_t;

// Process management functions
void process_init(void);
process_t* process_create(void (*entry_point)(void), uint32 is_user_mode);
process_t* process_get_current(void);
void process_switch(process_t* next);

#endif
