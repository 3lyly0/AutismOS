# AutismOS Documentation

Welcome to the AutismOS documentation. This directory contains detailed guides for understanding, building, and contributing to the project.

## Documents

| Document | Description |
|----------|-------------|
| [Architecture](architecture.md) | System architecture, boot flow, and how all components connect |
| [Building](building.md) | How to set up the toolchain, compile, and run AutismOS |
| [Kernel](kernel.md) | Kernel internals: GDT, IDT, ISR, PIC, memory management |
| [Drivers](drivers.md) | Hardware drivers: keyboard, mouse, disk, sound |
| [Style Guide](style-guide.md) | Code conventions and naming rules for the project |

## Quick Start

```bash
# Build the ISO
make

# Run in QEMU
qemu-system-i386 -cdrom autismos.iso

# Clean build artifacts
make clean
```

## Project Status

See [TODO.md](../TODO.md) for the full roadmap and current progress.
