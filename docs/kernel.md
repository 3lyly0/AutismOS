# AutismOS — Kernel Internals

This document explains how each kernel subsystem works.

---

## 1. Global Descriptor Table (GDT)

**Files:** `kernel/gdt.c`, `asm/load_gdt.asm`, `include/gdt.h`

### What it does
The GDT defines memory segments that the CPU uses in Protected Mode. It tells the CPU what memory regions exist and what permissions they have.

### Current setup
AutismOS uses a **flat memory model** — all segments cover the full 4GB address space (base=0, limit=0xFFFFF with granularity=4KB).

| Entry | Selector | Type | Ring | Description |
|-------|----------|------|------|-------------|
| 0 | 0x00 | Null | — | Required null descriptor |
| 1 | 0x08 | Code | 0 | Kernel code segment |
| 2 | 0x10 | Data | 0 | Kernel data segment |
| 3 | 0x18 | Code | 3 | User code segment (unused) |
| 4 | 0x20 | Data | 3 | User data segment (unused) |

### How it works
1. `gdt_init()` fills the GDT entries with segment descriptors.
2. `load_gdt()` (assembly) loads the GDTR register with `lgdt`.
3. A far jump (`jmp 0x08:flush`) reloads the CS register.
4. Data segment registers (DS, ES, FS, GS, SS) are set to 0x10.

### Key notes
- 8 entries are allocated but only 5 are used. The remaining 3 are reserved for future use (e.g., TSS for multitasking).
- The assembly file also contains `detect_v86` and `enter_v86` functions for Virtual 8086 mode, but these are **not used anywhere**.

---

## 2. Interrupt Descriptor Table (IDT)

**Files:** `kernel/idt.c`, `asm/load_idt.asm`, `include/idt.h`

### What it does
The IDT maps interrupt numbers (0–255) to handler functions. When an interrupt fires, the CPU looks up the IDT to find which function to call.

### Current setup

| Vectors | Count | Type | Source |
|---------|-------|------|--------|
| 0–31 | 32 | Exceptions | CPU (divide by zero, page fault, etc.) |
| 32–47 | 16 | IRQs | Hardware (keyboard, timer, etc.) |
| 128 | 1 | Syscall | Software interrupt (`int 0x80`) |

### How it works
1. `idt_init()` registers all 49 handlers using `idt_set_entry()`.
2. Each entry contains: handler address, code segment (0x08), and flags.
3. `load_idt()` (assembly) loads the IDTR register with `lidt`.

### ⚠️ Known issue
The flags are set as `0x8E | 0x60 = 0xEE`, which makes DPL=3 for ALL entries. This means user-mode code can trigger any interrupt — only `int 0x80` (syscall) should be DPL=3.

---

## 3. Interrupt Service Routines (ISR)

**Files:** `kernel/isr.c`, `asm/exception.asm`, `asm/irq.asm`, `include/isr.h`

### Exception handling flow
```
CPU Exception
  → exception.asm stub (push error code + int number, save registers)
    → isr_exception_handler(REGISTERS regs)
      → If int_no < 32: infinite halt loop (should print error message)
```

### IRQ handling flow
```
Hardware Interrupt
  → irq.asm stub (push IRQ number + int number, save registers)
    → isr_irq_handler(REGISTERS *regs)
      → Call registered handler function (e.g., keyboard_handler)
      → Send EOI to PIC
```

### Registering a handler
```c
// Register keyboard handler for IRQ 1 (vector 33)
isr_irq_register(33, keyboard_handler);
```

### REGISTERS struct
```c
typedef struct {
    uint32 ds;                                    // Data segment
    uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32 int_no, err_code;                      // Pushed by stub
    uint32 eip, cs, eflags, useresp, ss;          // Pushed by CPU
} REGISTERS;
```

---

## 4. Programmable Interrupt Controller (8259 PIC)

**Files:** `kernel/8259_pic.c`, `include/8259_pic.h`

### What it does
The 8259 PIC is the chip that manages hardware interrupts. There are two PICs cascaded together (master + slave), handling 16 IRQ lines total.

### Remapping
By default, IRQs 0–7 map to vectors 0–7 (which conflict with CPU exceptions). `pic_init()` remaps them:

| IRQ | Before | After |
|-----|--------|-------|
| 0–7 | Vector 0–7 | Vector 32–39 (0x20–0x27) |
| 8–15 | Vector 8–15 | Vector 40–47 (0x28–0x2F) |

### End of Interrupt (EOI)
After handling an IRQ, the handler must send an EOI signal:
- IRQ 0–7: Send EOI to master PIC only
- IRQ 8–15: Send EOI to both slave and master PIC

---

## 5. Video (VGA Text Mode)

**Files:** `kernel/video.c`, `include/video.h`

### What it does
Writes text directly to VGA text-mode memory at `0xB8000`. The screen is 80 columns × 25 rows.

### Memory format
Each character on screen = 2 bytes:
```
Byte 0: ASCII character code
Byte 1: Attribute (foreground color | background color << 4)
```

### Available functions
| Function | Description |
|----------|-------------|
| `print(str)` | Print a string at current cursor position |
| `print_char(c)` | Print a single character |
| `print_hex(value)` | Print a 32-bit value in hexadecimal |
| `clear_screen()` | Clear the entire screen |
| `update_cursor(row, col)` | Move the hardware cursor |
| `newline()` | Move to the next line |

### ⚠️ Known issues
- Cursor position tracked by two separate variable sets (see TODO.md Phase 0)
- `clear_screen()` sets wrong attribute byte (0x00 instead of 0x07)
- `update_cursor()` signature mismatch between header and implementation

---

## 6. Memory Management

**Files:** `kernel/memory.c`, `include/memory.h`

### What it does
Manages physical memory using a **bitmap allocator**. Each bit represents one 4KB page.

### Constants
| Name | Value | Description |
|------|-------|-------------|
| `PAGE_SIZE` | 4096 (4KB) | Size of one memory page |
| `MEMORY_SIZE` | 0x1000000 (16MB) | Total managed memory |
| `NUM_PAGES` | 4096 | Total number of pages (16MB / 4KB) |

### Available functions
| Function | Description |
|----------|-------------|
| `memory_init()` | Initialize all pages as free |
| `allocate_page()` | Find and return a free page (returns physical address) |
| `free_page(addr)` | Mark a page as free |
| `get_used_pages()` | Count allocated pages |
| `get_total_pages()` | Returns NUM_PAGES (4096) |

### How it works
```
Bitmap: [0][0][0][1][1][0][0]...
         │  │  │  │  │  │  │
Page:    0  1  2  3  4  5  6
         │           │  │
         Reserved     Allocated
```

- Bit = 0 → page is free
- Bit = 1 → page is allocated
- Page 0 is always reserved (NULL pointer protection)

### ⚠️ Known issues
- Kernel memory pages are **not protected** — they can be allocated over
- No `malloc`/`free` — only page-level allocation
- No paging hardware enabled — everything is physical addresses

---

## 7. I/O Ports

**Files:** `kernel/io_ports.c`, `include/io_ports.h`

### What it does
Provides functions to read from and write to hardware I/O ports using x86 `in` and `out` instructions.

### Available functions
| Function | Width | Direction |
|----------|-------|-----------|
| `inportb(port)` | 8-bit | Read |
| `outportb(port, val)` | 8-bit | Write |
| `inports(port)` / `inportw(port)` | 16-bit | Read |
| `outports(port, val)` / `outportw(port, val)` | 16-bit | Write |
| `inportl(port)` | 32-bit | Read |
| `outportl(port, val)` | 32-bit | Write |

> **Note:** `inports`/`inportw` and `outports`/`outportw` are duplicates — they do the same thing. This should be cleaned up (see TODO.md Phase 1).
