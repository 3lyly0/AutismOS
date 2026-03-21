#include "scheduler.h"
#include "kheap.h"
#include "string.h"
#include "video.h"
#include "kernel.h"

/* Global scheduler state */
static struct {
    sched_task_t* current;                    /* Currently running task */
    sched_task_t* run_queues[SCHED_PRIORITY_LEVELS]; /* Priority queues */
    sched_task_t task_pool[SCHED_MAX_TASKS];  /* Task pool */
    sched_stats_t stats;
    uint64_t ticks;                           /* Total timer ticks */
    uint8_t initialized;
} g_sched;

/* External timer ticks from kernel */
extern volatile uint64 g_timer_ticks;

/* Find a free slot in the task pool */
static sched_task_t* find_free_slot(void) {
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (g_sched.task_pool[i].task == NULL && 
            g_sched.task_pool[i].state == SCHED_STATE_TERMINATED) {
            return &g_sched.task_pool[i];
        }
    }
    return NULL;
}

/* Add task to the end of a priority queue */
static void enqueue_task(sched_task_t* stask) {
    if (!stask) return;
    
    int prio = stask->priority;
    if (prio >= SCHED_PRIORITY_LEVELS) {
        prio = SCHED_PRIORITY_NORMAL;
    }
    
    stask->next = NULL;
    stask->prev = NULL;
    
    if (!g_sched.run_queues[prio]) {
        g_sched.run_queues[prio] = stask;
    } else {
        /* Find end of queue */
        sched_task_t* current = g_sched.run_queues[prio];
        while (current->next) {
            current = current->next;
        }
        current->next = stask;
        stask->prev = current;
    }
    
    g_sched.stats.ready_tasks++;
}

/* Remove task from priority queue */
static void dequeue_task(sched_task_t* stask) {
    if (!stask) return;
    
    int prio = stask->priority;
    if (prio >= SCHED_PRIORITY_LEVELS) {
        prio = SCHED_PRIORITY_NORMAL;
    }
    
    if (stask->prev) {
        stask->prev->next = stask->next;
    } else {
        g_sched.run_queues[prio] = stask->next;
    }
    
    if (stask->next) {
        stask->next->prev = stask->prev;
    }
    
    stask->next = NULL;
    stask->prev = NULL;
    
    if (g_sched.stats.ready_tasks > 0) {
        g_sched.stats.ready_tasks--;
    }
}

/* Find highest priority ready task */
static sched_task_t* find_next_task(void) {
    for (int prio = SCHED_PRIORITY_LEVELS - 1; prio >= 0; prio--) {
        if (g_sched.run_queues[prio]) {
            return g_sched.run_queues[prio];
        }
    }
    return NULL;
}

/* Find sched_task for a given task_t */
static sched_task_t* find_sched_task(task_t* task) {
    if (!task) return NULL;
    
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (g_sched.task_pool[i].task == task) {
            return &g_sched.task_pool[i];
        }
    }
    return NULL;
}

int scheduler_init(void) {
    if (g_sched.initialized) {
        return 0;
    }
    
    memset(&g_sched, 0, sizeof(g_sched));
    g_sched.current = NULL;
    g_sched.ticks = 0;
    
    /* Clear run queues */
    for (int i = 0; i < SCHED_PRIORITY_LEVELS; i++) {
        g_sched.run_queues[i] = NULL;
    }
    
    /* Clear task pool */
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        g_sched.task_pool[i].task = NULL;
        g_sched.task_pool[i].state = SCHED_STATE_TERMINATED;
    }
    
    g_sched.initialized = 1;
    
    debug_print("Scheduler: initialized\n");
    return 0;
}

int scheduler_add_task(task_t* task, process_t* process, sched_priority_t priority) {
    if (!g_sched.initialized || !task) {
        return -1;
    }
    
    sched_task_t* stask = find_free_slot();
    if (!stask) {
        debug_print("Scheduler: no free task slots\n");
        return -1;
    }
    
    stask->task = task;
    stask->process = process;
    stask->priority = priority;
    stask->state = SCHED_STATE_READY;
    stask->time_slice = SCHED_TIME_SLICE;
    stask->total_cpu_time = 0;
    stask->wake_time = 0;
    stask->wait_data = NULL;
    stask->next = NULL;
    stask->prev = NULL;
    
    /* Link task back to scheduler info */
    task->sched_data = stask;
    
    enqueue_task(stask);
    g_sched.stats.total_tasks++;
    
    debug_print("Scheduler: added task at priority ");
    debug_print_hex(priority);
    debug_print("\n");
    
    return 0;
}

void scheduler_remove_task(task_t* task) {
    if (!task) return;
    
    sched_task_t* stask = find_sched_task(task);
    if (!stask) return;
    
    dequeue_task(stask);
    stask->task = NULL;
    stask->state = SCHED_STATE_TERMINATED;
    
    if (g_sched.stats.total_tasks > 0) {
        g_sched.stats.total_tasks--;
    }
}

sched_task_t* scheduler_get_current(void) {
    return g_sched.current;
}

void scheduler_yield(void) {
    if (!g_sched.initialized || !g_sched.current) {
        return;
    }
    
    sched_task_t* current = g_sched.current;
    sched_task_t* next = find_next_task();
    
    /* No other task to run */
    if (!next || next == current) {
        return;
    }
    
    /* Move current task to end of its queue */
    if (current->state == SCHED_STATE_RUNNING) {
        current->state = SCHED_STATE_READY;
        dequeue_task(current);
        enqueue_task(current);
    }
    
    /* Switch to next task */
    next->state = SCHED_STATE_RUNNING;
    next->time_slice = SCHED_TIME_SLICE;
    g_sched.current = next;
    
    g_sched.stats.context_switches++;
    
    /* Context switch would happen here */
    /* For now, we just update the current pointer */
    /* Real implementation would save/restore registers */
}

void scheduler_block(void* reason) {
    if (!g_sched.current) return;
    
    g_sched.current->state = SCHED_STATE_BLOCKED;
    g_sched.current->wait_data = reason;
    dequeue_task(g_sched.current);
    g_sched.stats.blocked_tasks++;
    
    scheduler_yield();
}

void scheduler_wake(task_t* task) {
    sched_task_t* stask = find_sched_task(task);
    if (!stask || stask->state != SCHED_STATE_BLOCKED) {
        return;
    }
    
    stask->state = SCHED_STATE_READY;
    stask->wait_data = NULL;
    enqueue_task(stask);
    
    if (g_sched.stats.blocked_tasks > 0) {
        g_sched.stats.blocked_tasks--;
    }
}

void scheduler_sleep(uint32_t ms) {
    if (!g_sched.current || ms == 0) {
        return;
    }
    
    if (ms > SCHED_MAX_SLEEP_MS) {
        ms = SCHED_MAX_SLEEP_MS;
    }
    
    uint64_t wake_tick = g_sched.ticks + scheduler_ms_to_ticks(ms);
    g_sched.current->state = SCHED_STATE_SLEEPING;
    g_sched.current->wake_time = (uint32_t)wake_tick;
    dequeue_task(g_sched.current);
    
    scheduler_yield();
}

void scheduler_tick(void) {
    g_sched.ticks++;
    g_sched.stats.timer_ticks++;
    
    /* Wake up sleeping tasks */
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        sched_task_t* stask = &g_sched.task_pool[i];
        if (stask->state == SCHED_STATE_SLEEPING && 
            stask->wake_time <= (uint32_t)g_sched.ticks) {
            stask->state = SCHED_STATE_READY;
            stask->wake_time = 0;
            enqueue_task(stask);
        }
    }
    
    /* Check time slice */
    if (g_sched.current) {
        g_sched.current->total_cpu_time++;
        
        if (g_sched.current->time_slice > 0) {
            g_sched.current->time_slice--;
        }
        
        if (g_sched.current->time_slice == 0) {
            scheduler_yield();
        }
    }
}

void scheduler_set_priority(task_t* task, sched_priority_t priority) {
    sched_task_t* stask = find_sched_task(task);
    if (!stask) return;
    
    /* Remove from old queue */
    if (stask->state == SCHED_STATE_READY) {
        dequeue_task(stask);
    }
    
    stask->priority = priority;
    
    /* Re-add to new queue */
    if (stask->state == SCHED_STATE_READY) {
        enqueue_task(stask);
    }
}

sched_priority_t scheduler_get_priority(task_t* task) {
    sched_task_t* stask = find_sched_task(task);
    if (!stask) return SCHED_PRIORITY_NORMAL;
    return stask->priority;
}

void scheduler_exit(int exit_code) {
    (void)exit_code;
    
    if (!g_sched.current) return;
    
    g_sched.current->state = SCHED_STATE_ZOMBIE;
    dequeue_task(g_sched.current);
    
    if (g_sched.stats.total_tasks > 0) {
        g_sched.stats.total_tasks--;
    }
    
    scheduler_yield();
}

void scheduler_get_stats(sched_stats_t* stats) {
    if (stats) {
        *stats = g_sched.stats;
        stats->initialized = g_sched.initialized;
    }
}

void scheduler_dump(void) {
    debug_print("\n=== Scheduler State ===\n");
    debug_print("Current task: 0x");
    debug_print_hex((uint32_t)g_sched.current);
    debug_print("\nTotal tasks: ");
    debug_print_hex(g_sched.stats.total_tasks);
    debug_print("\nReady tasks: ");
    debug_print_hex(g_sched.stats.ready_tasks);
    debug_print("\nBlocked tasks: ");
    debug_print_hex(g_sched.stats.blocked_tasks);
    debug_print("\nContext switches: ");
    debug_print_hex(g_sched.stats.context_switches);
    debug_print("\nTimer ticks: ");
    debug_print_hex((uint32_t)g_sched.ticks);
    debug_print("\n");
    
    /* Show tasks by priority */
    for (int prio = SCHED_PRIORITY_LEVELS - 1; prio >= 0; prio--) {
        if (g_sched.run_queues[prio]) {
            debug_print("Priority ");
            debug_print_hex(prio);
            debug_print(": ");
            sched_task_t* t = g_sched.run_queues[prio];
            while (t) {
                debug_print("0x");
                debug_print_hex((uint32_t)t->task);
                debug_print(" ");
                t = t->next;
            }
            debug_print("\n");
        }
    }
    debug_print("=======================\n");
}

int scheduler_is_initialized(void) {
    return g_sched.initialized;
}

uint32_t scheduler_runnable_count(void) {
    return g_sched.stats.ready_tasks;
}

uint64_t scheduler_get_ticks(void) {
    return g_sched.ticks;
}

uint32_t scheduler_ticks_to_ms(uint64_t ticks) {
    /* Assuming 100Hz timer (10ms per tick) */
    return (uint32_t)(ticks * 10);
}

uint64_t scheduler_ms_to_ticks(uint32_t ms) {
    /* Assuming 100Hz timer */
    return ms / 10;
}