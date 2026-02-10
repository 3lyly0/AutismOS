# AutismOS Project Organization

This document describes the organizational structure of the AutismOS codebase.

## Directory Structure

```
AutismOS/
├── boot/              # Bootloader and boot configuration
│   ├── bootloader.asm # Assembly bootloader code
│   ├── grub.cfg       # GRUB configuration
│   └── linker.ld      # Linker script
│
├── asm/               # Assembly source files
│   ├── exception.asm  # Exception handlers
│   ├── irq.asm        # Interrupt request handlers
│   ├── load_gdt.asm   # Global Descriptor Table loader
│   ├── load_idt.asm   # Interrupt Descriptor Table loader
│   ├── syscall.asm    # System call entry points
│   └── user_program.asm # User mode program helpers
│
├── kernel/            # Kernel source code (organized by subsystem)
│   ├── core/          # Core kernel functionality
│   │   ├── kernel.c   # Main kernel initialization and demo
│   │   ├── memory.c   # Memory management (kmalloc, kfree, paging)
│   │   ├── process.c  # Process management
│   │   └── task.c     # Task scheduling and switching
│   │
│   ├── arch/          # Architecture-specific code (x86)
│   │   ├── 8259_pic.c # Programmable Interrupt Controller
│   │   ├── gdt.c      # Global Descriptor Table
│   │   ├── idt.c      # Interrupt Descriptor Table
│   │   ├── io_ports.c # I/O port operations
│   │   └── isr.c      # Interrupt Service Routines
│   │
│   ├── ipc/           # Inter-Process Communication
│   │   ├── ipc.c      # IPC message passing
│   │   └── shm.c      # Shared memory management
│   │
│   ├── syscall/       # System call implementation
│   │   ├── syscall.c  # System call dispatcher
│   │   └── usermode.c # User mode support
│   │
│   └── browser/       # Browser-specific functionality
│       ├── html.c     # HTML parser
│       ├── layout.c   # Layout engine and rendering
│       └── network.c  # Network stack (HTTP)
│
├── drivers/           # Hardware drivers (organized by device type)
│   ├── input/         # Input device drivers
│   │   ├── input.c    # Generic input handling
│   │   ├── keyboard.c # Keyboard driver
│   │   └── mouse.c    # Mouse driver
│   │
│   ├── video/         # Video drivers
│   │   └── video.c    # VGA text mode driver
│   │
│   ├── storage/       # Storage device drivers
│   │   └── disk.c     # Disk I/O driver
│   │
│   └── audio/         # Audio drivers
│       └── sound.c    # PC speaker driver
│
├── lib/               # Utility libraries
│   └── string.c       # String manipulation functions
│
└── include/           # Header files
    ├── 8259_pic.h     # PIC definitions
    ├── disk.h         # Disk I/O interface
    ├── gdt.h          # GDT structures
    ├── html.h         # HTML parser interface
    ├── idt.h          # IDT structures
    ├── input.h        # Input handling interface
    ├── io_ports.h     # I/O port macros
    ├── ipc.h          # IPC interface
    ├── isr.h          # ISR definitions
    ├── kernel.h       # Kernel main interface
    ├── keyboard.h     # Keyboard interface
    ├── layout.h       # Layout engine interface
    ├── memory.h       # Memory management interface
    ├── mouse.h        # Mouse interface
    ├── multiboot.h    # Multiboot specification
    ├── network.h      # Network interface
    ├── process.h      # Process management interface
    ├── shm.h          # Shared memory interface
    ├── sound.h        # Sound interface
    ├── string.h       # String functions
    ├── syscall.h      # System call definitions
    ├── task.h         # Task scheduling interface
    ├── types.h        # Type definitions
    ├── user_program.h # User program interface
    ├── usermode.h     # User mode interface
    └── video.h        # Video interface
```

## Organization Principles

### Kernel Subdirectories

The kernel code is organized into logical subsystems:

1. **core/** - Essential kernel functions that manage the system at a high level
   - Process and task management
   - Memory allocation and paging
   - Main kernel initialization

2. **arch/** - Hardware and architecture-specific code for x86
   - Descriptor tables (GDT, IDT)
   - Interrupt handling
   - I/O port access
   - PIC configuration

3. **ipc/** - Inter-process communication mechanisms
   - Message passing
   - Shared memory

4. **syscall/** - System call infrastructure
   - System call dispatcher
   - User mode transitions

5. **browser/** - Browser-specific features (unique to AutismOS)
   - HTML parsing
   - Layout engine
   - Network stack

### Driver Subdirectories

Hardware drivers are organized by device type:

1. **input/** - Input devices
   - Keyboard
   - Mouse
   - Generic input event handling

2. **video/** - Display devices
   - VGA text mode

3. **storage/** - Storage devices
   - Disk I/O

4. **audio/** - Audio devices
   - PC speaker

## Benefits of This Organization

1. **Clarity** - Related files are grouped together, making it easier to find code
2. **Modularity** - Clear separation between subsystems
3. **Maintainability** - Changes to one subsystem are isolated from others
4. **Scalability** - Easy to add new drivers or kernel features in their appropriate locations
5. **Educational Value** - The structure clearly shows the major components of an operating system

## Build System

The Makefile has been updated to reflect this organization. Source files are referenced with their full paths (e.g., `kernel/core/kernel.c`, `drivers/input/keyboard.c`), while object files continue to be placed in a flat `build/` directory for simplicity.

This allows the source code to be well-organized while keeping the build process straightforward.
