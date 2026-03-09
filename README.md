# AutismOS

AutismOS is a small educational operating system for learning bootloading, interrupts, memory, devices, and GUI architecture on x86.

**Latest Achievements:**
- ✅ **Pixel GUI Desktop** - VGA mode `320x200` desktop with real windows
- ✅ **Window Manager Foundation** - Focus, z-order, drag, resize, close, and content layout helpers
- ✅ **Animated Boot Sequence**
- ✅ **Real Networking Stack**

---

## **Current Features**

### ✅ Desktop GUI

AutismOS now boots into a pixel-based desktop instead of a text-mode shell.

#### Current GUI Features
- **VGA mode `320x200x256`**
- **Desktop shell** with taskbar and start menu
- **Window manager basics**:
  - Focus and z-order
  - Click-to-focus
  - Close button
  - Drag windows by the title bar
  - Resize windows from the lower-right corner
- **Bundled apps**:
  - Notepad
  - Calculator
  - System Info
- **Shared desktop chrome**:
  - Unified title bars
  - Shared content rect calculation
  - Per-window minimum sizes
- **Custom software rendering** with a backbuffer and bitmap font

### ✅ Boot Animation

Animated boot sequence for a cleaner startup experience.

#### Boot Animation Features
- **Multi-stage animated boot**
- **Progress bar**
- **Large AutismOS logo**
- **Transition into the desktop shell**

### ✅ Mouse Input

Mouse handling is now based on a single primary PS/2 path tuned for the desktop workflow.

#### Mouse Improvements
- **Less aggressive movement**
- **Clamped deltas** to prevent huge jumps
- **Screen bounds protection**
- **Better fit for GUI interaction**

### ✅ Real Networking Stack

Full implementation of a minimal TCP/IP networking stack from hardware to ICMP:

#### Layer 1 - Hardware
- **RTL8139 NIC driver** with PCI device detection
- **Polling-based packet I/O** (no IRQ required)
- **TX/RX buffer management**
- **MAC address reading**

#### Layer 2 - Ethernet
- **Frame construction and parsing**
- **EtherType handling** (ARP, IP)
- **Local MAC address management**

#### Layer 3 - ARP
- **ARP request/reply handling**
- **8-entry ARP cache** (fixed-size array)
- **Automatic ARP resolution**

#### Layer 4 - IP
- **IPv4 packet construction**
- **IP checksum calculation**
- **Static configuration**: 10.0.2.15/24, Gateway: 10.0.2.2
- **Next-hop routing** (local subnet vs gateway)

#### Layer 5 - ICMP
- **ICMP Echo Request (ping)**
- **ICMP Echo Reply handling**
- **Real ping to external IPs** (e.g., 8.8.8.8)
- **Timeout handling** with configurable wait

#### Network Configuration
- **Static IP**: 10.0.2.15
- **Netmask**: 255.255.255.0
- **Gateway**: 10.0.2.2
- No DHCP, No DNS
- HTTP browser functionality disabled (TCP not implemented)

### ✅ System Foundations

- **Bootloader and protected-mode kernel startup**
- **GDT / IDT / ISR setup**
- **Paging and heap bootstrap**
- **Task and process scaffolding**
- **IPC and shared memory primitives**
- **PCI and RTL8139 device discovery**

---

## **Getting Started**

This guide will help you set up the environment, build the operating system, and run it using Docker and QEMU.

---

### **Prerequisites**

Make sure you have the following installed on your system:
- **Docker**: For building and running the development environment.
- **QEMU**: For running the operating system.

---

### **Setup and Usage**

#### **1. Clone the Repository**
Clone the AutismOS repository to your local machine:
```bash
git clone https://github.com/3lyly0/AutismOS.git
cd AutismOS
```
---

#### **2. Build and Run the Docker Environment**
Use Docker to set up the development environment:
```bash
docker run --rm -it -v "${PWD}:/env" dev
```

- **Explanation**:
  - `--rm`: Automatically removes the container after exiting.
  - `-it`: Runs the container interactively.
  - `-v "${PWD}:/env"`: Mounts the current directory to `/env` inside the container.

#### **3. Build the Operating System**
Inside the Docker container, run the following command to build the OS:
```bash
make
```

This will:
- Compile the source code.
- Generate the ISO file (`autismos.iso`).

#### **4. Run the Operating System**
After building the OS, you can run it using QEMU.

**Base command:**
```bash
qemu-system-x86_64 -cdrom autismos.iso -device isa-debug-exit,iobase=0x8900,iosize=0x04
```

**Recommended for a larger QEMU window:**
```bash
qemu-system-x86_64 -cdrom autismos.iso -device isa-debug-exit,iobase=0x8900,iosize=0x04 -display gtk,zoom-to-fit=on
```

**Recommended for the largest view:**
```bash
qemu-system-x86_64 -cdrom autismos.iso -device isa-debug-exit,iobase=0x8900,iosize=0x04 -display gtk,zoom-to-fit=on -full-screen
```

- `-cdrom autismos.iso`: Boot from the generated ISO.
- `-device isa-debug-exit,...`: Let QEMU exit cleanly when the guest requests it.
- `-display gtk,zoom-to-fit=on`: Makes the tiny guest display easier to see on modern monitors.
- `-full-screen`: Best option if you want the desktop to feel much larger immediately.

---

### **Rebuilding and Running the OS**
If you make changes to the source code, follow these steps to rebuild and run the OS:

1. **Rebuild the OS**:
   ```bash
   make clean
   make
   ```

2. **Run the OS**:
   ```bash
   qemu-system-x86_64 -cdrom autismos.iso -device isa-debug-exit,iobase=0x8900,iosize=0x04 -display gtk,zoom-to-fit=on
   ```

---

### **What You'll See**

When you run AutismOS, you'll see:

- An animated boot screen
- A pixel desktop shell
- A taskbar and start menu
- Large desktop windows for Notepad, Calculator, and System Info
- A visible software mouse cursor

#### Notes About Window Size

The guest resolution is currently **320x200**, so without QEMU scaling it will look physically small on modern displays.

If the desktop feels tiny, use one of these:
- `-display gtk,zoom-to-fit=on`
- `-full-screen`
- QEMU's own zoom controls if your frontend supports them

Inside the guest itself, the bundled application windows are now larger by default and support dragging and resizing.


---

### **Documentation**

- **`STEP6_7_IMPLEMENTATION.md`**: Complete Step 6 & 7 implementation guide
- **`STEP5_IPC.md`**: IPC implementation details
- **`IMPLEMENTATION_SUMMARY.md`**: Feature summary
- **`ARCHITECTURE.md`**: System architecture

---

### **Graphics API**

The graphics system is now centered around a pixel framebuffer in VGA mode:

#### Core Graphics Functions
- `graphics_init()` - switch to VGA graphics mode
- `graphics_present()` - copy the backbuffer to VRAM
- `graphics_clear_screen(color)` - clear the entire backbuffer
- `graphics_clear_region(x, y, w, h, color)` - clear part of the backbuffer

#### Drawing Primitives
- `draw_pixel(x, y, color)` - plot one pixel
- `draw_line(x0, y0, x1, y1, color)` - draw a line
- `draw_rect(x, y, w, h, color)` - rectangle outline
- `draw_filled_rect(x, y, w, h, color)` - Solid filled rectangle
- `draw_char(x, y, ch, color)` - Single character
- `draw_text(x, y, text, color)` - Text string
- `draw_text_scaled(x, y, text, color, scale)` - Scaled text
- `draw_char_with_bg(x, y, ch, fg, bg)` - Character with custom colors
- `draw_text_with_bg(x, y, text, fg, bg)` - Text with custom colors
- `draw_cursor(x, y, visible)` - Software mouse cursor
- `draw_progress_bar(x, y, w, percent, color)` - Visual progress indicator

#### Windowing Helpers
- `desktop_get_window_content_rect(...)`
- `desktop_hit_test_window(...)`
- `desktop_move_window(...)`
- `desktop_resize_window(...)`

---

### **File Structure**
- **`boot/`**: Contains bootloader code and linker scripts.
- **`asm/`**: Assembly language source files (GDT, IDT, IRQ, syscalls, exceptions).
- **`kernel/`**: Contains the kernel source code, organized into subdirectories:
  - **`kernel/core/`**: Core kernel functions (kernel.c, memory.c, process.c, task.c)
  - **`kernel/arch/`**: Architecture-specific code (gdt.c, idt.c, isr.c, io_ports.c, 8259_pic.c)
  - **`kernel/ipc/`**: Inter-process communication (ipc.c, shm.c)
  - **`kernel/syscall/`**: System call implementation (syscall.c, usermode.c)
  - **`kernel/browser/`**: Browser-specific functionality (html.c, layout.c, network.c)
  - **`kernel/ux/`**: User experience layer (ux.c, desktop.c)
- **`drivers/`**: Hardware drivers, organized by device type:
- **`drivers/input/`**: Input devices (keyboard.c, mouse.c, **mouse_smooth.c** ⭐NEW⭐, input.c)
- **`drivers/video/`**: Video drivers (video.c, graphics.c, ui.c, **boot_animation.c** ⭐NEW⭐)
  - **`drivers/storage/`**: Storage devices (disk.c)
  - **`drivers/audio/`**: Audio devices (sound.c)
  - **`drivers/network/`**: Network stack (rtl8139.c, ethernet.c, arp.c, ip.c, icmp.c, tcp.c)
- **`apps/`**: User applications (notepad.c, calculator.c, sysinfo.c)
- **`lib/`**: Utility libraries (string.c)
- **`include/`**: Header files for shared definitions and declarations.
- **`Makefile`**: Build script for compiling the OS.
- **`autismos.iso`**: The generated ISO file for the OS.

---

### **Troubleshooting**

#### **1. Docker Volume Issues**
If you cannot see your files inside the Docker container, ensure you are using the correct volume mount syntax:
- **Windows (PowerShell)**:
  ```bash
  docker run --rm -it -v "${PWD}:/env" dev
  ```
- **Windows (Command Prompt)**:
  ```bash
  docker run --rm -it -v "%cd%:/env" dev
  ```
- **Linux/macOS**:
  ```bash
  docker run --rm -it -v "$(pwd):/env" dev
  ```

#### **2. QEMU Window Is Too Small**
The guest still renders at `320x200`, so the host window can look tiny.

Try:
```bash
qemu-system-x86_64 -cdrom autismos.iso -device isa-debug-exit,iobase=0x8900,iosize=0x04 -display gtk,zoom-to-fit=on
```

Or:
```bash
qemu-system-x86_64 -cdrom autismos.iso -device isa-debug-exit,iobase=0x8900,iosize=0x04 -display gtk,zoom-to-fit=on -full-screen
```

#### **3. QEMU Exits Immediately**
Ensure the ISO file (`autismos.iso`) exists in the current directory. If not, rebuild the OS using:
```bash
make
```

---

### **Contributing**
Contributions are welcome! Feel free to open issues or submit pull requests to improve AutismOS.

---

### **License**
This project is licensed under the GNU General Public License (GPL) Version 3. See the `LICENSE` file for details.

---

## **Current Interaction Model**

When the QEMU window has focus:
- Click windows to focus them
- Drag by the title bar
- Resize from the lower-right corner
- Click the start button to open the launcher
- Press `1`, `2`, or `3` while the launcher is open to spawn apps
- Type into the focused app

