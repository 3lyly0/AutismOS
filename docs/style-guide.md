# AutismOS — Code Style Guide

This document defines the coding conventions for AutismOS. All new code should follow these rules. Existing code should be updated to match when modified.

---

## Language & Environment

- **Language:** C99 (with GNU extensions where needed for inline assembly)
- **Environment:** Freestanding (`-ffreestanding -nostdlib`)
- **No standard library.** Do not use `#include <stdio.h>`, `<stdlib.h>`, `<string.h>`, etc. Use our custom implementations in `lib/` and `include/`.
- **Target:** i686 (32-bit x86), little-endian

---

## File Organization

### Directory structure
```
kernel/     → Core kernel code (runs at boot, manages hardware)
drivers/    → Hardware device drivers (keyboard, mouse, disk, sound)
lib/        → Utility libraries (string, memory operations)
include/    → Header files (one per .c file)
asm/        → Assembly files (interrupt stubs, CPU instructions)
boot/       → Boot-related files (bootloader, GRUB config, linker script)
docs/       → Documentation
```

### File naming
- All lowercase, underscores for spaces: `io_ports.c`, `8259_pic.c`
- One `.h` header per `.c` file, same base name
- Headers go in `include/`, source goes in its appropriate directory

---

## Naming Conventions

### Functions
```c
// snake_case — all lowercase with underscores
void memory_init(void);
void ata_read_sector(uint32 lba, uint8 *buffer);
int  create_file(const char *name);
```

### Variables
```c
// snake_case for local and global variables
uint32 cursor_x;
uint32 page_count;
static int shift_pressed;
```

### Constants & Macros
```c
// UPPER_SNAKE_CASE
#define PAGE_SIZE       4096
#define MEMORY_SIZE     0x1000000
#define VGA_ADDRESS     0xB8000
#define KEYBOARD_PORT   0x60
```

### Type definitions
```c
// lowercase with _t suffix (or without for legacy types)
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef int            BOOL;   // Legacy — keep for compatibility
```

### Structs
```c
// PascalCase for struct names, snake_case for fields
typedef struct {
    char name[32];
    uint32 size;
    uint32 start_lba;
} FileEntry;
```

---

## Formatting

### Indentation
- **4 spaces** (no tabs)

### Braces
```c
// Opening brace on the same line
void function_name(void) {
    if (condition) {
        // ...
    } else {
        // ...
    }
}
```

### Line length
- Soft limit: **100 characters**
- Hard limit: **120 characters**

### Spacing
```c
// Space after keywords
if (condition) { ... }
for (int i = 0; i < n; i++) { ... }
while (running) { ... }

// No space after function names
print("hello");
outportb(0x60, value);

// Spaces around operators
int result = a + b;
if (flags & MASK) { ... }
```

---

## Headers

### Include guards
```c
#ifndef KEYBOARD_H
#define KEYBOARD_H

// declarations...

#endif
```

### Include order
```c
// 1. Our own headers first
#include "types.h"
#include "io_ports.h"

// 2. Related module headers
#include "video.h"
#include "memory.h"

// Never include system headers (<stdio.h>, <stdlib.h>, etc.)
```

---

## Comments

### File headers
```c
/*
 * keyboard.c — PS/2 Keyboard Driver
 *
 * Handles keyboard input via IRQ 1. Translates scancodes to ASCII.
 */
```

### Function documentation
```c
/*
 * Allocate a single 4KB page of physical memory.
 * Returns the physical address of the page, or 0 if out of memory.
 */
uint32 allocate_page(void);
```

### Inline comments
```c
outportb(0x20, 0x20);  // Send EOI to master PIC
```

### TODO markers
```c
// TODO: Add bounds checking for cursor position
// FIXME: This loop should decrement, not increment
// HACK: Busy-wait because we don't have a timer yet
```

---

## Inline Assembly

Use GCC extended inline assembly syntax:

```c
static inline uint8 inportb(uint16 port) {
    uint8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
```

Rules:
- Always use `volatile` for I/O operations
- Use constraint letters correctly (`"a"` for eax, `"Nd"` for port, etc.)
- Add a comment explaining what the instruction does if it's not obvious

---

## Error Handling

Since we have no exceptions or error recovery:
1. Return error codes (0 = success, -1 = failure) for functions that can fail.
2. Use a `panic()` function for unrecoverable errors (print message + halt).
3. Validate inputs at function boundaries.

```c
int create_file(const char *name) {
    if (name == 0) return -1;
    if (file_count >= MAX_FILES) return -1;
    // ...
    return 0;  // Success
}
```

---

## Git Commit Messages

```
<type>: <short description>

Types:
  fix:      Bug fix
  feat:     New feature
  clean:    Code cleanup / refactor
  docs:     Documentation changes
  build:    Build system changes

Examples:
  fix: Correct keyboard scancode loop direction
  feat: Add shell with help and clear commands
  clean: Unify cursor variables in video.c
  docs: Add kernel internals documentation
```
