#ifndef IPC_H
#define IPC_H

#include "types.h"

// Message structure - fixed size, no pointers
// Only integers to avoid memory bugs
typedef struct message {
    uint32 sender_pid;    // PID of sending process
    uint32 type;          // Message type (user-defined)
    uint32 data1;         // First data field
    uint32 data2;         // Second data field
} message_t;

// Message queue configuration
#define MESSAGE_QUEUE_SIZE 16  // Messages per process

// Message queue structure - circular buffer
typedef struct message_queue {
    message_t messages[MESSAGE_QUEUE_SIZE];
    uint32 read_index;     // Where to read next
    uint32 write_index;    // Where to write next
    uint32 count;          // Number of messages in queue
} message_queue_t;

// Message queue operations
void message_queue_init(message_queue_t* queue);
int message_queue_enqueue(message_queue_t* queue, message_t* msg);
int message_queue_dequeue(message_queue_t* queue, message_t* msg);
int message_queue_is_empty(message_queue_t* queue);
int message_queue_is_full(message_queue_t* queue);

#endif
