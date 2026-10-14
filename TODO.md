# AutismOS — TODO & Roadmap

> **Last updated:** October 2026
>
> This document tracks all planned work for AutismOS, ordered by priority.
> Each phase must be completed before moving to the next.
> Items marked with `[x]` are done. Items marked with `[ ]` are pending.

---

## Phase 0 — Critical Bug Fixes

> These bugs can crash the system or cause invisible output.
> **Nothing else should be worked on until these are fixed.**

- [x] **keyboard.c — Infinite loop in `get_scancode()`**
  - File: `drivers/keyboard.c`
  - Bug: `for (i = 1000; i > 0; i++)` — the loop increments `i` instead of decrementing it.
  - Fix: Change `i++` to `i--`.
  - Impact: Without this fix, the loop runs until integer overflow.

- [x] **video.c — Cursor variable conflict**
  - File: `kernel/video.c`
  - Bug: `print()` uses `static unsigned int row, col` (local), while `clear_screen()` uses `cursor_x, cursor_y` (global). They are never synced.
  - Fix: Use a single pair of global cursor variables everywhere.
  - Impact: After `clear_screen()`, text prints at the old position instead of (0,0).

- [x] **video.h — `update_cursor()` signature mismatch**
  - File: `include/video.h` vs `kernel/video.c`
  - Bug: Header declares `void update_cursor()` (no params), but the definition is `void update_cursor(int row, int col)`.
  - Fix: Update the header to match: `void update_cursor(int row, int col);`
  - Impact: `clear_screen()` calls `update_cursor()` without arguments — undefined behavior.

- [x] **video.c — `clear_screen()` produces invisible text**
  - File: `kernel/video.c`
  - Bug: `clear_screen()` sets attribute bytes to `0x00` (black on black).
  - Fix: Set attribute bytes to `0x07` (light grey on black) or `0x0F` (white on black).
  - Impact: Screen appears blank after clearing because text color is black.

---

## Phase 1 — Code Cleanup & Consistency

> Clean up the codebase so it compiles warning-free and follows consistent conventions.

### 1.1 — Remove invalid includes
- [x] Remove `#include <stdio.h>` from `kernel/isr.c`
- [x] Remove `#include <stdio.h>` from `drivers/mouse.c`
- [x] Remove `#include <string.h>` from `drivers/disk.c` (use our custom `include/string.h`)

### 1.2 — Unify type definitions
- [x] Pick one naming style: either `uint8` or `uint8_t` — not both.
- [x] Update `include/types.h` to define the chosen style.
- [x] Update `include/io_ports.h` to use consistent types.
- [x] Remove duplicate I/O functions (`inports`/`inportw` are identical).

### 1.3 — Fix minor standard-compliance issues
- [x] `lib/string.c` — Make `memcmp()` return 0 on match (standard behavior). Currently returns 0 on mismatch.
- [x] `lib/string.c` — Make `strcpy()` return `char*` instead of `int`.

### 1.4 — Clean up linker script
- [x] `boot/linker.ld` — Remove the hardcoded Multiboot header (it's already in `bootloader.asm`).
- [x] Fix the duplicate `.rodata` section definition.

### 1.5 — Clean up kernel.c
- [x] Make `kmain()` accept Multiboot parameters: `void kmain(unsigned long magic, unsigned long addr)`.
- [x] Remove debug `print()` calls from `kernel/memory.c`.
- [x] Add a proper panic function that prints an error message (replace the silent infinite loop in `isr.c`).

### 1.6 — Fix IDT privilege levels
- [x] `kernel/idt.c` — Remove `| 0x60` from `idt_set_entry`. Only syscall entry (128) should have DPL=3. All others should be DPL=0.

### 1.7 — Protect kernel memory
- [x] `kernel/memory.c` — Mark kernel pages (0x100000 and above, up to `_kernel_end`) as reserved so they cannot be allocated.

### 1.8 — Build system improvements
- [x] Add `make clean` target to the Makefile.
- [x] Add `-Wall -Wextra -Werror` to compiler flags.
- [x] Verify the project compiles with zero warnings after all cleanups.

---

## Phase 2 — Core OS Infrastructure

> These are the foundational systems that everything else depends on.
> Build them in order — each one builds on the previous.

### 2.1 — PIT Timer Driver
- [x] Program the Programmable Interval Timer (PIT) — Channel 0, ~1000 Hz (1ms tick).
- [x] Create `drivers/timer.c` and `include/timer.h`.
- [x] Register IRQ0 handler.
- [x] Implement a global tick counter (`timer_get_ticks()`).
- [x] Implement `sleep_ms(uint32 ms)` using the tick counter.
- [x] Replace the busy-wait loop in `drivers/sound.c` with `sleep_ms()`.

> **Why first?** Accurate timing is needed for everything: sleep, scheduling, disk timeouts, etc.

### 2.2 — Heap Memory Allocator (malloc/free)
- [x] Create `kernel/heap.c` and `include/heap.h` (implemented via `kernel/core/kheap.c` and `include/kheap.h`).
- [x] Implement a simple block allocator on top of the existing page allocator.
- [x] Functions: `void* kmalloc(size_t size)`, `void kfree(void* ptr)`.
- [x] Add `void* kcalloc(size_t count, size_t size)` — allocate and zero.
- [x] Add `void* krealloc(void* ptr, size_t new_size)` — resize allocation.
- [x] Test: allocate, use, and free memory blocks of various sizes.

> **Why second?** Dynamic memory allocation is required for buffers, strings, data structures, and almost everything that follows.

### 2.3 — Paging & Virtual Memory
- [x] Create `kernel/paging.c` and `include/paging.h` (implemented via `kernel/core/memory.c` and `kernel/core/vmm.c`).
- [x] Set up a page directory and page tables (identity-map first 16MB).
- [x] Enable paging via CR0/CR3.
- [x] Implement page fault handler (ISR 14).
- [x] Map kernel space as supervisor-only (ring 0).
- [x] Add functions: `map_page()`, `unmap_page()`, `get_physical_addr()`.

> **Why third?** Paging provides memory protection — prevents user code from overwriting the kernel.

### 2.4 — Input System Overhaul
- [x] Create a proper keyboard input buffer (ring buffer, ~256 bytes).
- [x] Support reading a full line of input (with backspace, enter).
- [x] Add arrow key support (Left, Right for cursor movement within input).
- [x] Add Ctrl+C handling (cancel current input).
- [x] Create `char* readline(const char* prompt)` — print prompt, read a line, return it.

> **Why fourth?** The shell (Phase 3) needs proper line-based input, not single-character echo.

---

## Phase 3 — Essential Features

> With the infrastructure in place, build the features that make it a real OS.

### 3.1 — Shell (Command Interpreter)
- [ ] Create `kernel/shell.c` and `include/shell.h`.
- [ ] Parse user input into command + arguments.
- [ ] Built-in commands:
  - [ ] `help` — List all commands with descriptions.
  - [ ] `clear` — Clear the screen.
  - [ ] `echo <text>` — Print text.
  - [ ] `beep` — Play a beep sound.
  - [ ] `meminfo` — Show memory usage (total, used, free pages).
  - [ ] `uptime` — Show system uptime (requires PIT timer).
  - [ ] `reboot` — Reboot the system.
  - [ ] `shutdown` — Halt the CPU.
- [ ] Command history (Up/Down arrows to recall previous commands).
- [ ] Unknown command error message: `"Unknown command: <cmd>. Type 'help' for a list of commands."`

### 3.2 — Activate Mouse Driver
- [ ] Un-comment `mouse_init()` in `kernel.c`.
- [ ] Fix: Save and restore the character under the mouse pointer (don't overwrite with space).
- [ ] Add bounds checking for cursor position.
- [ ] Add shell command: `mouse on/off` — Enable/disable mouse.

### 3.3 — Activate Disk & File System
- [ ] Fix `disk.c` — `start_lba` off-by-one bug (`file_count` incremented before use).
- [ ] Add `ata_wait()` timeout to prevent infinite hangs.
- [ ] Persist the file table to disk (sector 0 = superblock with file table).
- [ ] Add shell commands:
  - [ ] `ls` — List all files.
  - [ ] `cat <file>` — Print file contents.
  - [ ] `create <file>` — Create a new empty file.
  - [ ] `write <file> <text>` — Write text to a file.
  - [ ] `rm <file>` — Delete a file.
  - [ ] `stat <file>` — Show file info (size, LBA).

### 3.4 — Formatted Output
- [ ] Implement `printf()` with format specifiers: `%d`, `%u`, `%x`, `%s`, `%c`, `%%`.
- [ ] Implement `sprintf()` for writing to a buffer.
- [ ] Replace all manual `print()` + `print_hex()` chains with `printf()`.

### 3.5 — VGA Improvements
- [ ] Add color support: `set_text_color(fg, bg)`.
- [ ] Colored shell prompt (e.g., green `AutismOS>` prompt).
- [ ] Add `print_at(row, col, text)` — print at specific position.
- [ ] Status bar at the top or bottom of the screen (time, memory usage).

---

## Phase 4 — Advanced Features (Future)

> These are stretch goals. Each one is a significant project on its own.

### 4.1 — Multitasking
- [ ] Implement a Task State Segment (TSS).
- [ ] Create a process/task struct (registers, stack, state).
- [ ] Implement context switching (save/restore registers).
- [ ] Round-robin scheduler using PIT timer.
- [ ] Support at least 2 concurrent tasks (kernel tasks).

### 4.2 — User Mode
- [ ] Switch to Ring 3 for user programs.
- [ ] Implement system calls via `int 0x80` (the syscall entry is already registered).
- [ ] Basic syscalls: `sys_write`, `sys_read`, `sys_exit`.
- [ ] Separate kernel and user address spaces using paging.

### 4.3 — Real File System (FAT12/16)
- [ ] Implement FAT12 or FAT16 file system.
- [ ] Read/write files from a real disk image.
- [ ] Support directories.
- [ ] Boot from a FAT-formatted disk.

### 4.4 — ELF Loader
- [ ] Parse ELF binary headers.
- [ ] Load ELF executables into memory.
- [ ] Shell command: `exec <program>` — Load and run a user program.

### 4.5 — Networking (Long-term)
- [ ] NE2000 or RTL8139 network card driver.
- [ ] ARP and IP protocols.
- [ ] UDP sockets.
- [ ] Simple `ping` command.

---

## Quick Reference — File Map

```
AutismOS/
├── boot/
│   ├── bootloader.asm   # Multiboot entry point, sets up stack, calls kmain
│   ├── grub.cfg         # GRUB bootloader configuration
│   └── linker.ld        # Linker script — memory layout
├── asm/
│   ├── exception.asm    # 32 CPU exception stubs + syscall (int 128)
│   ├── irq.asm          # 16 hardware interrupt stubs (IRQ 0–15)
│   ├── load_gdt.asm     # Load GDT register + far jump
│   └── load_idt.asm     # Load IDT register
├── kernel/
│   ├── kernel.c         # kmain() — system initialization & main loop
│   ├── gdt.c            # Global Descriptor Table setup
│   ├── idt.c            # Interrupt Descriptor Table setup
│   ├── isr.c            # Interrupt Service Routines (dispatch)
│   ├── 8259_pic.c       # Programmable Interrupt Controller (PIC)
│   ├── video.c          # VGA text-mode driver (80x25)
│   ├── memory.c         # Physical memory manager (bitmap allocator)
│   └── io_ports.c       # Low-level port I/O (in/out)
├── drivers/
│   ├── keyboard.c       # PS/2 keyboard driver
│   ├── mouse.c          # PS/2 mouse driver (currently disabled)
│   ├── disk.c           # ATA PIO disk driver + simple file system
│   └── sound.c          # PC speaker driver (beep)
├── lib/
│   └── string.c         # String & memory utilities
├── include/             # Header files for all modules
├── docs/                # Project documentation
├── Makefile             # Build system
├── README.md            # Project overview
├── TODO.md              # This file
└── LICENSE              # GPL v3
```

---

## Contributing

When working on a task:
1. Pick an item from the earliest incomplete phase.
2. Create a branch: `feature/<short-name>` or `fix/<short-name>`.
3. Mark the item as `[x]` when done.
4. Test in QEMU: `qemu-system-i386 -cdrom autismos.iso`.
5. Commit with a clear message referencing the TODO item.
