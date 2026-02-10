# Step 5: IPC Implementation

## Overview

This implementation adds Inter-Process Communication (IPC) capabilities to AutismOS, enabling isolated processes to communicate safely through kernel-mediated message passing.

## Key Components

### 1. Message Structure (`message_t`)
- Fixed-size structure (no pointers)
- Contains: sender_pid, type, data1, data2
- All fields are 32-bit integers to avoid memory bugs

### 2. Message Queue (`message_queue_t`)
- Circular buffer with 16 message slots per process
- Each process has its own inbox
- Supports enqueue/dequeue operations

### 3. IPC Syscalls

#### SYS_SEND (syscall #2)
- Sends a message to another process
- Parameters:
  - EBX = target PID
  - ECX = pointer to message_t
- Returns: 0 on success, -1 if queue full
- Automatically wakes blocked receiver

#### SYS_RECV (syscall #3)
- Receives a message (blocking)
- Parameters:
  - EBX = pointer to message_t buffer
- Returns: 0 on success, -1 if no message
- Blocks task until message arrives

#### SYS_POLL (syscall #4)
- Checks for messages (non-blocking)
- Parameters:
  - EBX = pointer to message_t buffer
- Returns: 0 if message received, -1 if no message
- Never blocks

## Browser Architecture Demonstration

The implementation includes a demonstration of browser-like architecture:

### Browser Process (PID 1)
- Simulates UI/control process
- Sends RENDER requests to Renderer (PID 0)
- Polls for FRAME_READY responses

### Renderer Process (PID 0)
- Simulates HTML/layout engine
- Created first, receives PID 0
- Polls for RENDER requests
- Sends FRAME_READY responses back

### Monitor Process (PID 2)
- Tracks system execution
- Displays process activity counters

## Event-Driven Model

Processes use an event loop pattern:

```c
while (1) {
    message_t msg;
    if (sys_poll_msg(&msg) == 0) {
        // Handle message
        handle_event(&msg);
    }
    // Do work
    yield();
}
```

This demonstrates the event-driven architecture required for browser implementations.

## Testing

Build and run:
```bash
make
qemu-system-x86_64 -cdrom autismos.iso
```

You will see:
- Processes created with message queues
- Browser and Renderer communicating
- Counters incrementing showing active message passing
- System remains responsive

## Compliance with Step 5 Requirements

✅ Multiple user processes exist
✅ Processes exchange messages via kernel
✅ Kernel mediates all IPC
✅ No shared memory
✅ No crashes
✅ System remains responsive
✅ Event-driven execution model

## Future Enhancements (Step 6)

- Shared memory for framebuffers
- Zero-copy rendering
- Window compositing
