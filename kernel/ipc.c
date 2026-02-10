#include "ipc.h"
#include "string.h"

// Initialize message queue
void message_queue_init(message_queue_t* queue) {
    memset(queue, 0, sizeof(message_queue_t));
    queue->read_index = 0;
    queue->write_index = 0;
    queue->count = 0;
}

// Check if queue is empty
int message_queue_is_empty(message_queue_t* queue) {
    return queue->count == 0;
}

// Check if queue is full
int message_queue_is_full(message_queue_t* queue) {
    return queue->count >= MESSAGE_QUEUE_SIZE;
}

// Enqueue a message (add to queue)
// Returns 0 on success, -1 if queue is full
int message_queue_enqueue(message_queue_t* queue, message_t* msg) {
    if (message_queue_is_full(queue)) {
        return -1;  // Queue full
    }
    
    // Copy message to queue
    queue->messages[queue->write_index] = *msg;
    
    // Update write index (circular)
    queue->write_index = (queue->write_index + 1) % MESSAGE_QUEUE_SIZE;
    queue->count++;
    
    return 0;  // Success
}

// Dequeue a message (remove from queue)
// Returns 0 on success, -1 if queue is empty
int message_queue_dequeue(message_queue_t* queue, message_t* msg) {
    if (message_queue_is_empty(queue)) {
        return -1;  // Queue empty
    }
    
    // Copy message from queue
    *msg = queue->messages[queue->read_index];
    
    // Update read index (circular)
    queue->read_index = (queue->read_index + 1) % MESSAGE_QUEUE_SIZE;
    queue->count--;
    
    return 0;  // Success
}
