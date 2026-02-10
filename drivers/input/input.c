#include "input.h"
#include "ipc.h"
#include "process.h"

// Target process for input events (UI/Browser process)
static uint32 input_target_pid = 1;  // Browser is typically PID 1

// Initialize input subsystem
void input_init(void) {
    // Input is delivered via IPC to the browser process
    // No special initialization needed
}

// Send keyboard event to browser via IPC
void input_send_key_event(uint32 type, uint32 keycode) {
    // Find the browser process
    process_t* browser = process_find_by_pid(input_target_pid);
    if (!browser) {
        return;
    }
    
    // Create input event message
    message_t msg;
    msg.sender_pid = 0;  // Kernel/System
    msg.type = type;     // INPUT_EVENT_KEY_DOWN or INPUT_EVENT_KEY_UP
    msg.data1 = keycode; // Key code
    msg.data2 = 0;       // Reserved
    
    // Send to browser's inbox
    message_queue_enqueue(&browser->inbox, &msg);
    
    // Wake browser if waiting
    if (browser->main_thread && browser->main_thread->state == TASK_WAITING) {
        browser->main_thread->state = TASK_READY;
    }
}

// Send mouse event to browser via IPC
void input_send_mouse_event(uint32 type, uint32 button, uint32 x, uint32 y) {
    // Find the browser process
    process_t* browser = process_find_by_pid(input_target_pid);
    if (!browser) {
        return;
    }
    
    // Create input event message
    message_t msg;
    msg.sender_pid = 0;     // Kernel/System
    msg.type = type;        // INPUT_EVENT_MOUSE_*
    msg.data1 = button;     // Mouse button
    msg.data2 = (x << 16) | (y & 0xFFFF);  // Pack X and Y coordinates
    
    // Send to browser's inbox
    message_queue_enqueue(&browser->inbox, &msg);
    
    // Wake browser if waiting
    if (browser->main_thread && browser->main_thread->state == TASK_WAITING) {
        browser->main_thread->state = TASK_READY;
    }
}
