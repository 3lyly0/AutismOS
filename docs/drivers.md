# AutismOS — Hardware Drivers

This document covers all hardware drivers in the `drivers/` directory.

---

## 1. PS/2 Keyboard Driver

**Files:** `drivers/keyboard.c`, `include/keyboard.h`

### Overview
Handles input from a standard PS/2 keyboard via IRQ 1 (interrupt vector 33). The driver reads scancodes from I/O port `0x60` and translates them to ASCII characters.

### How it works
1. `keyboard_init()` registers `keyboard_handler` for IRQ 33.
2. When a key is pressed, the PIC fires IRQ 1.
3. `keyboard_handler()` reads the scancode from port `0x60`.
4. The scancode is stored in a global variable for `kb_getchar()` to read.

### Supported features
- ✅ Lowercase letters (a–z)
- ✅ Numbers (0–9)
- ✅ Common symbols (!, @, #, etc.)
- ✅ Space, Enter, Backspace, Tab
- ✅ Left Shift (hold for uppercase / symbols)
- ✅ Caps Lock (toggle)

### Not supported
- ❌ Right Shift
- ❌ Ctrl, Alt, Super keys
- ❌ Function keys (F1–F12)
- ❌ Arrow keys
- ❌ Numpad
- ❌ Key repeat (typematic)

### API

```c
// Initialize the keyboard driver (call once at boot)
void keyboard_init(void);

// Get a single character (blocking — waits until a key is pressed)
char kb_getchar(void);
```

### Scancode table
The driver uses Scan Code Set 1 (the default for PS/2 keyboards). Key mappings are defined in `include/keyboard.h` as two arrays:
- `lowercase[]` — Normal key mappings
- `uppercase[]` — Shift/Caps Lock key mappings

### ⚠️ Known bug
`get_scancode()` has an infinite loop: `for (i = 1000; i > 0; i++)` — should be `i--`. See TODO.md Phase 0.

---

## 2. PS/2 Mouse Driver

**Files:** `drivers/mouse.c`, `include/mouse.h`

### Status: 🔴 Disabled
The mouse driver is fully written but `mouse_init()` is commented out in `kernel.c`.

### Overview
Handles a PS/2 mouse via IRQ 12 (interrupt vector 44). The driver reads 3-byte packets from the mouse and updates a cursor position on screen.

### How it works
1. `mouse_init()` enables the auxiliary mouse device on the PS/2 controller.
2. Sends commands to enable data reporting.
3. Registers `mouse_handler` for IRQ 44.
4. Each mouse event sends a 3-byte packet:
   - Byte 1: Button state + sign bits + overflow bits
   - Byte 2: X movement (delta)
   - Byte 3: Y movement (delta)

### Cursor rendering
The mouse cursor is rendered as an `'X'` character in VGA text mode. When the cursor moves:
1. The old position is cleared (replaced with a space character).
2. The new position gets an `'X'`.

### ⚠️ Known issues
- Clearing the old position overwrites whatever text was there (loses content).
- Uses `#include <stdio.h>` which doesn't exist in freestanding environment.
- No bounds checking — cursor can go off-screen.

### API

```c
// Initialize the mouse driver and enable data reporting
void mouse_init(void);
```

---

## 3. ATA Disk Driver (PIO Mode)

**Files:** `drivers/disk.c`, `include/disk.h`

### Status: 🔴 Written but never used
The disk driver and simple file system are implemented but not connected to `kernel.c`.

### Overview
Reads and writes sectors to/from an ATA hard disk using PIO (Programmed I/O) mode — the simplest disk access method.

### ATA PIO Ports

| Port | Read | Write |
|------|------|-------|
| 0x1F0 | Data | Data |
| 0x1F1 | Error | Features |
| 0x1F2 | — | Sector count |
| 0x1F3 | — | LBA low |
| 0x1F4 | — | LBA mid |
| 0x1F5 | — | LBA high |
| 0x1F6 | — | Drive/Head |
| 0x1F7 | Status | Command |

### Low-level API

```c
// Wait until the disk is ready (BSY=0, DRQ=1)
void ata_wait(void);

// Read one 512-byte sector from disk into buffer
void ata_read_sector(uint32 lba, uint8 *buffer);

// Write one 512-byte sector from buffer to disk
void ata_write_sector(uint32 lba, uint8 *buffer);
```

### Simple File System
On top of the ATA driver, there's a minimal in-memory file system:

```c
typedef struct {
    char name[32];      // File name
    uint32 size;        // File size in bytes
    uint32 start_lba;   // Starting sector on disk
} FileEntry;
```

### File System API

```c
// Create a new file (allocates 1 sector)
int create_file(const char *name);

// Read file contents into buffer
int read_file(const char *name, uint8 *buffer);

// Write buffer contents to file
int write_file(const char *name, uint8 *data, uint32 size);

// List all files (prints to screen)
void list_files(void);
```

### ⚠️ Known issues
- File table is in-memory only — lost on reboot.
- `start_lba` off-by-one: first file starts at LBA 2 instead of LBA 1.
- No protection against files overlapping on disk.
- `ata_wait()` can hang forever if no disk is present.
- Maximum 64 files.
- Each file limited to 1 sector (512 bytes).

---

## 4. PC Speaker (Sound)

**Files:** `drivers/sound.c`, `include/sound.h`

### Overview
Produces a simple beep using the PC Speaker via the Programmable Interval Timer (PIT).

### How it works
1. Program PIT Channel 2 to generate a square wave at the desired frequency.
2. Enable the speaker by setting bits 0 and 1 of port `0x61`.
3. Wait (busy loop).
4. Disable the speaker by clearing bits 0 and 1 of port `0x61`.

### Formula
```
PIT divisor = 1193180 / frequency_in_hz
```

### API

```c
// Play a 1000 Hz beep for a short duration
void beep(void);
```

### ⚠️ Known issues
- Duration is a busy-wait loop (`for` loop counting to 90,000,000) — actual time depends on CPU speed.
- Only supports one hardcoded frequency (1000 Hz).
- Should use a timer interrupt for accurate timing (see TODO.md Phase 2).

### Future improvements
```c
// Planned API:
void play_tone(uint32 frequency_hz, uint32 duration_ms);
void stop_sound(void);
```
