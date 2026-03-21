#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stddef.h>
#include "task.h"
#include "process.h"

/**
 * Process Scheduler
 * 
 * A priority-based round-robin scheduler with:
 * - Multiple priority levels
 * - Time slicing
 * - Preemptive multitasking
 * - Sleep/wake mechanism
 */

/* Scheduler configuration */
#define SCHED_MAX_TASKS       64
#define SCHED_PRIORITY_LEVELS 8
#define SCHED_TIME_SLICE      10      /* Timer ticks per time slice */
#define SCHED_MAX_SLEEP_MS    60000   /* Maximum sleep time in ms */

/* Priority levels */
typedef enum {
    SCHED_PRIORITY_IDLE    = 0,
    SCHED_PRIORITY_LOW     = 1,
    SCHED_PRIORITY_NORMAL  = 4,
    SCHED_PRIORITY_HIGH    = 6,
    SCHED_PRIORITY_REALTIME = 7,
    SCHED_PRIORITY_KERNEL  = 7
} sched_priority_t;

/* Task states */
typedef enum {
    SCHED_STATE_READY = 0,
    SCHED_STATE_RUNNING,
    SCHED_STATE_BLOCKED,
    SCHED_STATE_SLEEPING,
    SCHED_STATE_ZOMBIE,
    SCHED_STATE_TERMINATED
} sched_state_t;

/* Scheduler statistics */
typedef struct {
    uint32_t total_tasks;       /* Total tasks in system */
    uint32_t running_tasks;     /* Currently running tasks */
    uint32_t ready_tasks;       /* Ready to run */
    uint32_t blocked_tasks;     /* Blocked waiting */
    uint32_t context_switches;  /* Total context switches */
    uint32_t timer_ticks;       /* Total timer ticks */
    uint8_t  initialized;       /* Is scheduler initialized? */
} sched_stats_t;

/* Extended task info for scheduler */
typedef struct sched_task {
    task_t* task;               /* Pointer to base task */
    process_t* process;         /* Associated process */
    sched_priority_t priority;  /* Current priority */
    sched_state_t state;        /* Current state */
    uint32_t time_slice;        /* Remaining time slice */
    uint32_t total_cpu_time;    /* Total CPU time used */
    uint32_t wake_time;         /* Wake time for sleeping tasks */
    void* wait_data;            /* Data for wait condition */
    struct sched_task* next;    /* Next task in queue */
    struct sched_task* prev;    /* Previous task in queue */
} sched_task_t;

/* ============== Public API ============== */

/**
 * Initialize the scheduler
 * 
 * @return           0 on success, -1 on failure
 */
int scheduler_init(void);

/**
 * Add a task to the scheduler
 * 
 * @param task       Task to add
 * @param process    Associated process (can be NULL for kernel tasks)
 * @param priority   Task priority
 * @return           0 on success, -1 on failure
 */
int scheduler_add_task(task_t* task, process_t* process, sched_priority_t priority);

/**
 * Remove a task from the scheduler
 * 
 * @param task       Task to remove
 */
void scheduler_remove_task(task_t* task);

/**
 * Get current running task
 * 
 * @return           Pointer to current sched_task, or NULL
 */
sched_task_t* scheduler_get_current(void);

/**
 * Request a context switch
 * Called from timer interrupt
 */
void scheduler_yield(void);

/**
 * Block current task
 * Task will not be scheduled until woken
 * 
 * @param reason     Pointer to wait reason (for debugging)
 */
void scheduler_block(void* reason);

/**
 * Wake a blocked task
 * 
 * @param task       Task to wake
 */
void scheduler_wake(task_t* task);

/**
 * Sleep for specified milliseconds
 * 
 * @param ms         Milliseconds to sleep
 */
void scheduler_sleep(uint32_t ms);

/**
 * Set task priority
 * 
 * @param task       Task to modify
 * @param priority   New priority
 */
void scheduler_set_priority(task_t* task, sched_priority_t priority);

/**
 * Get task priority
 * 
 * @param task       Task to query
 * @return           Current priority
 */
sched_priority_t scheduler_get_priority(task_t* task);

/**
 * Terminate current task
 * 
 * @param exit_code  Exit code
 */
void scheduler_exit(int exit_code);

/**
 * Get scheduler statistics
 * 
 * @param stats      Pointer to stats structure to fill
 */
void scheduler_get_stats(sched_stats_t* stats);

/**
 * Dump scheduler state for debugging
 */
void scheduler_dump(void);

/**
 * Check if scheduler is initialized
 * 
 * @return           1 if initialized, 0 otherwise
 */
int scheduler_is_initialized(void);

/**
 * Get number of runnable tasks
 * 
 * @return           Number of tasks in READY state
 */
uint32_t scheduler_runnable_count(void);

/* ============== Timer Integration ============== */

/**
 * Called from timer interrupt
 * Decrements time slice and triggers context switch if needed
 */
void scheduler_tick(void);

/**
 * Get current timer ticks
 * 
 * @return           Total timer ticks since boot
 */
uint64_t scheduler_get_ticks(void);

/**
 * Convert ticks to milliseconds
 * 
 * @param ticks      Timer ticks
 * @return           Milliseconds
 */
uint32_t scheduler_ticks_to_ms(uint64_t ticks);

/**
 * Convert milliseconds to ticks
 * 
 * @param ms         Milliseconds
 * @return           Timer ticks
 */
uint64_t scheduler_ms_to_ticks(uint32_t ms);

#endif /* SCHEDULER_H */