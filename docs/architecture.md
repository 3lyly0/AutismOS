# AutismOS — System Architecture

## Overview

AutismOS is a 32-bit x86 operating system that runs in Protected Mode. It uses GRUB as its bootloader and operates directly on hardware (bare-metal, freestanding environment — no standard C library).

## Boot Flow

```mermaid
flowchart TD
    BIOS["BIOS"] --> GRUB["GRUB\nReads grub.cfg, loads kernel ELF"]
    GRUB --> START["bootloader.asm :: _start"]

    START --> S1["Verify Multiboot magic number"]
    START --> S2["Set up kernel stack ~100KB"]
    START --> S3["Push multiboot info + magic"]
    S1 & S2 & S3 --> KMAIN["kernel.c :: kmain"]

    KMAIN --> GDT["gdt_init — Memory segments"]
    GDT --> IDT["idt_init — Interrupt handlers"]
    IDT --> PIC["pic_init — Hardware interrupts"]
    PIC --> MEM["memory_init — Page allocator"]
    MEM --> KB["keyboard_init — Keyboard input"]
    KB --> BEEP["beep — Startup sound"]
    BEEP --> LOOP["Main loop\nEcho keyboard input"]
```

## Memory Layout

```mermaid
flowchart LR
    subgraph MEM["Physical Memory Layout"]
        direction TB
        A["0x00000000\nReserved — NULL page"]
        B["0x00000000 – 0x000A0000\nReal Mode IVT + BIOS Data\n(unused in protected mode)"]
        C["0x000B8000\nVGA Text-Mode Video Memory\n80×25 characters"]
        D["0x000C0000 – 0x000FFFFF\nBIOS ROM Area"]
        E["0x00100000 — Kernel Start (1MB)\n.text | .rodata | .data | .bss\nKernel stack ~100KB"]
        F["_kernel_end\nFree Memory\n(managed by page allocator)"]
        G["0x01000000\nEnd of Managed Memory — 16MB"]

        A --> B --> C --> D --> E --> F --> G
    end
```

## Component Map

```mermaid
flowchart TB
    subgraph KERNEL["KERNEL"]
        K1["kernel.c\nMain Entry"]
        K2["gdt.c\nSegments"]
        K3["idt.c\nInterrupts"]
        K4["8259_pic.c\nHW Interrupts"]
        K5["isr.c\nDispatch"]
        K6["video.c\nVGA Output"]
        K7["memory.c\nPage Allocator"]
        K8["io_ports.c\nPort I/O"]
    end

    subgraph DRIVERS["DRIVERS"]
        D1["keyboard.c\nPS/2 Keyboard — IRQ 1"]
        D2["mouse.c\nPS/2 Mouse — IRQ 12\nDisabled"]
        D3["disk.c\nATA PIO\nUnused"]
        D4["sound.c\nPC Speaker"]
    end

    subgraph LIB["LIBRARY"]
        L1["string.c\nmemset, memcpy, strlen, strcmp, itoa..."]
    end

    subgraph ASM["ASSEMBLY"]
        A1["exception.asm\n32 Handlers"]
        A2["irq.asm\n16 IRQs"]
        A3["load_gdt.asm\nFar Jump"]
        A4["load_idt.asm\nLIDT"]
    end

    KERNEL --> DRIVERS
    KERNEL --> LIB
    KERNEL --> ASM
    DRIVERS --> LIB
```

## Interrupt Map

| IRQ | Vector | Source | Handler |
|-----|--------|--------|---------|
| — | 0–31 | CPU Exceptions | `exception.asm` → `isr_exception_handler()` |
| 0 | 32 | PIT Timer | (not yet implemented) |
| 1 | 33 | Keyboard | `keyboard_handler()` |
| 12 | 44 | Mouse | `mouse_handler()` (disabled) |
| — | 128 | Syscall | `exception.asm` (not yet implemented) |

## Privilege Levels

| Ring | Purpose | Status |
|------|---------|--------|
| Ring 0 | Kernel code & data | ✅ Active (GDT entries 1–2) |
| Ring 3 | User code & data | ⚠️ GDT entries defined (3–4) but not used yet |

## Key Design Decisions

1. **Flat memory model** — All segments span the full 4GB address space. No segmentation tricks.
2. **Physical memory only** — No paging enabled yet. All addresses are physical.
3. **Monolithic kernel** — Everything runs in kernel space (Ring 0).
4. **VGA text mode** — 80×25 character display. No graphics mode.
5. **Polling I/O** — Keyboard uses interrupt-driven input, but disk uses polling (PIO mode).
