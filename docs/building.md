# AutismOS — Building & Running

## Prerequisites

You need a **cross-compiler toolchain** for i686 (32-bit x86) and some utilities:

### Required Tools

| Tool | Purpose | Install (Ubuntu/Debian) |
|------|---------|------------------------|
| `i686-elf-gcc` | Cross-compiler for i686 | Build from source or use `apt` |
| `i686-elf-ld` | Linker | Comes with binutils |
| `nasm` | Assembler | `sudo apt install nasm` |
| `grub-mkrescue` | Create bootable ISO | `sudo apt install grub-pc-bin` |
| `xorriso` | ISO creation backend | `sudo apt install xorriso` |
| `mtools` | FAT filesystem tools | `sudo apt install mtools` |
| `qemu-system-i386` | x86 emulator for testing | `sudo apt install qemu-system-x86` |

### Quick Install (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install nasm grub-pc-bin xorriso mtools qemu-system-x86
```

For the cross-compiler, follow the [OSDev GCC Cross-Compiler guide](https://wiki.osdev.org/GCC_Cross-Compiler).

### Windows (WSL)

If you're on Windows, use WSL (Windows Subsystem for Linux):

```powershell
# Install WSL with Ubuntu
wsl --install -d Ubuntu

# Then follow the Ubuntu instructions above inside WSL
```

## Building

### Build the ISO

```bash
make
```

This will:
1. Assemble all `.asm` files with NASM (→ `.o` object files)
2. Compile all `.c` files with `i686-elf-gcc` (→ `.o` object files)
3. Link everything into `kernel.bin` using `boot/linker.ld`
4. Package `kernel.bin` + `boot/grub.cfg` into `autismos.iso` with `grub-mkrescue`

### Clean build artifacts

```bash
make clean
```

> **Note:** The current Makefile may not have a `clean` target yet — see TODO.md Phase 1.

## Running

### In QEMU (Recommended for development)

```bash
# Basic run
qemu-system-i386 -cdrom autismos.iso

# With serial output for debugging
qemu-system-i386 -cdrom autismos.iso -serial stdio

# With more memory (default is 128MB)
qemu-system-i386 -cdrom autismos.iso -m 256

# With GDB debugging support
qemu-system-i386 -cdrom autismos.iso -s -S
# Then in another terminal:
gdb kernel.bin -ex "target remote :1234"
```

### In VirtualBox

1. Create a new VM: Type = Other, Version = Other/Unknown
2. Memory: 64MB or more
3. No hard disk needed
4. Settings → Storage → Add optical drive → Select `autismos.iso`
5. Start the VM

### On Real Hardware (Advanced)

1. Write `autismos.iso` to a USB drive:
   ```bash
   sudo dd if=autismos.iso of=/dev/sdX bs=4M status=progress
   ```
2. Boot from the USB drive (change BIOS boot order).

> **Warning:** Running on real hardware is risky. Use a machine you don't care about.

## Build System Details

### Makefile Variables

| Variable | Value | Purpose |
|----------|-------|---------|
| `CC` | `i686-elf-gcc` | C compiler |
| `LD` | `i686-elf-ld` | Linker |
| `ASM` | `nasm` | Assembler |
| `CFLAGS` | `-m32 -ffreestanding -nostdlib` | Compile flags |

### Output Files

| File | Description |
|------|-------------|
| `kernel.bin` | Linked kernel binary (ELF format) |
| `autismos.iso` | Bootable ISO image (GRUB + kernel) |

## Debugging Tips

### Print Debugging
Use `print()` and `print_hex()` from `video.c` to output values to the screen.

### QEMU Monitor
Press `Ctrl+Alt+2` in QEMU to open the monitor console:
- `info registers` — Show CPU registers
- `info mem` — Show memory mappings
- `xp /16xw 0x100000` — Examine memory at address

### GDB Remote Debugging
```bash
# Terminal 1: Start QEMU with GDB server
qemu-system-i386 -cdrom autismos.iso -s -S

# Terminal 2: Connect GDB
gdb kernel.bin
(gdb) target remote :1234
(gdb) break kmain
(gdb) continue
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `grub-mkrescue` fails | Make sure `xorriso` and `mtools` are installed |
| QEMU shows "No bootable device" | Verify `grub.cfg` path matches Makefile output structure |
| Screen is blank after boot | Likely the `clear_screen()` bug — see TODO.md Phase 0 |
| Keyboard doesn't respond | Check that `keyboard_init()` is called and IRQ1 is unmasked |
