#include "input.h"
#include "ipc.h"
#include "process.h"

// Target process for legacy IPC-delivered input events.
// The active desktop shell handles input directly, but this path remains
// available for future user-space apps that consume queued events.
static uint32 input_target_pid = 1;

// Initialize input subsystem
void input_init(void) {
    // No special initialization needed
}

void input_send_key_event(uint32 type, uint32 keycode) {
    process_t* target = process_find_by_pid(input_target_pid);
    if (!target) {
        return;
    }
    
    // Create input event message
    message_t msg;
    msg.sender_pid = 0;  // Kernel/System
    msg.type = type;     // INPUT_EVENT_KEY_DOWN or INPUT_EVENT_KEY_UP
    msg.data1 = keycode; // Key code
    msg.data2 = 0;       // Reserved
    
    message_queue_enqueue(&target->inbox, &msg);
    
    if (target->main_thread && target->main_thread->state == TASK_WAITING) {
        target->main_thread->state = TASK_READY;
    }
}

void input_send_mouse_event(uint32 type, uint32 button, uint32 x, uint32 y) {
    process_t* target = process_find_by_pid(input_target_pid);
    if (!target) {
        return;
    }
    
    // Create input event message
    message_t msg;
    msg.sender_pid = 0;     // Kernel/System
    msg.type = type;        // INPUT_EVENT_MOUSE_*
    msg.data1 = button;     // Mouse button
    msg.data2 = (x << 16) | (y & 0xFFFF);  // Pack X and Y coordinates
    
    message_queue_enqueue(&target->inbox, &msg);
    
    if (target->main_thread && target->main_thread->state == TASK_WAITING) {
        target->main_thread->state = TASK_READY;
    }
}
