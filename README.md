# AutismOS

AutismOS is a minimalistic educational operating system designed for learning low-level system programming, with a focus on bootloading, interrupts, hardware interaction, and **browser-oriented architecture**.

**Latest Achievement: Steps 6 & 7 - Browser-Capable OS ✅**

---

## **Current Features**

### ✅ Steps 6 & 7: Browser-Capable Operating System (COMPLETE)

#### Step 6: Shared Memory, Graphics & Rendering
- **Shared memory syscalls** (SYS_SHM_CREATE, SYS_SHM_MAP, SYS_SHM_UNMAP)
- **Zero-copy framebuffer** rendering between processes
- **Framebuffer structure** (320×200 resolution, 32-bit RGBA)
- **Frame events** via IPC (FRAME_READY notifications)
- **Security**: Reference counting, PID ownership, size limits

#### Step 7: Input, Networking & Browser Core
- **Input delivery** via IPC (keyboard and mouse events)
- **URL parsing** for http:// protocol
- **HTTP/1.0 networking** (minimal GET requests)
- **HTML parser** supporting html, body, h1, p, a tags
- **Layout engine** with vertical flow (primitive but functional)
- **Text rendering** to framebuffer with fixed-width font
- **Browser main loop** with complete event-driven architecture
- **Complete pipeline**: URL → Network → Parse → Layout → Render → Display

### ✅ Previous Steps
- **Step 5**: IPC, Messaging & Event-Driven Execution
- **Step 0-4**: Process abstraction, memory management, task scheduling, interrupts, syscalls
- Bootloader and kernel initialization

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
After building the OS, you can run it using QEMU:
```bash
qemu-system-x86_64 -cdrom autismos.iso -device isa-debug-exit,iobase=0x8900,iosize=0x04
```

- **Explanation**:
  - `-cdrom autismos.iso`: Specifies the ISO file to boot from.
  - `-device isa-debug-exit,iobase=0x8900,iosize=0x04`: Allows QEMU to exit cleanly when the OS halts.

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
   qemu-system-x86_64 -cdrom autismos.iso -device isa-debug-exit,iobase=0x8900,iosize=0x04
   ```

---

### **What You'll See**

When you run AutismOS, you'll see:
- System initialization and memory setup
- **Steps 6 & 7 Demo**: Complete browser pipeline execution
- Renderer creating shared framebuffer
- Browser mapping framebuffer (zero-copy)
- HTML fetching, parsing, layout, and rendering
- Frame-ready notifications via IPC
- Three isolated processes working together

Example output:
```
=== Steps 6 & 7: Browser-Capable Operating System ===
Step 6: Shared memory, framebuffer rendering
Step 7: Input, networking, HTML parsing, layout

Renderer process: PID=0x00000000 (Network + HTML + Layout + Render)
Browser process: PID=0x00000001 (UI/Input/Compositor)

[Renderer] Created shared framebuffer ID: 0x00000001 (320x200)
[Renderer] Framebuffer mapped at: 0x00113000
[Browser] User entered URL: http://example.com/
[Renderer] Fetching http://example.com/
[Renderer] HTTP Response: 200 (151 bytes)
[Renderer] Parsing HTML...
[Renderer] Creating layout...
[Renderer] Rendering to framebuffer...
[Browser] Page rendered and displayed!
```

---

### **Documentation**

- **`STEP6_7_IMPLEMENTATION.md`**: Complete Step 6 & 7 implementation guide
- **`STEP5_IPC.md`**: IPC implementation details
- **`IMPLEMENTATION_SUMMARY.md`**: Feature summary
- **`ARCHITECTURE.md`**: System architecture

---

### **File Structure**
- **`boot/`**: Contains bootloader code.
- **`kernel/`**: Contains the kernel source code.
- **`drivers/`**: Contains drivers for hardware interaction (e.g., keyboard, disk).
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

#### **2. QEMU Exits Immediately**
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
