# AutismOS

AutismOS is a minimalistic educational operating system designed for learning low-level system programming, with a focus on bootloading, interrupts, hardware interaction, and **browser-oriented architecture**.

**Latest Achievement: Step 9 - Interactive Graphics UI ✅**

---

## **Current Features**

### ✅ Step 9: Interactive Graphics UI & In-Page Interaction (COMPLETE)

The transformation from text-based interface to true graphical interaction:

#### Graphics Primitives
- **Drawing operations** for rectangles, text, characters, and cursor
- **VGA text mode graphics** with color support
- **Partial redraw model** - only update changed regions
- **No full screen clears** - maintains visual stability

#### UI Components
- **Textbox widget** with visual borders and focus indication
- **Focus management** - only one element receives input at a time
- **Caret blinking** using timer interrupts (~500ms)
- **Visual feedback** - focused elements highlighted

#### Interactive Flow
- **URL input field** - users can type directly on screen
- **Enter key submission** - press Enter to navigate
- **In-page rendering** - content appears without mode switches
- **Live updates** - typed characters appear immediately
- **No debug logs** visible to user - clean experience

#### User Experience
- **Graphical boot screen** with ASCII art logo
- **Ready screen** with URL input and instructions
- **Content area** for page display
- **Keyboard-driven** interaction
- **Feels alive** - responsive and interactive

### ✅ Step 8: User Experience Kernel (COMPLETE)

The transformation from technical demo to real OS:

#### Clean Boot Experience
- **UX Kernel Module** manages the user experience layer
- **Boot screen** with AutismOS ASCII art logo
- **Clean UI** after boot showing system status and shortcuts
- **Silent mode** - debug logs only to serial port, not screen
- **No technical spam** - users see a clean interface, not kernel chatter

#### Control Loop & Interactivity
- **Keyboard shortcuts** for system control
  - `Alt+B` - Focus Browser
  - `Alt+Q` - Quit Application
- **Hotkey handler** integrated with keyboard driver
- **Immediate response** to user input

#### Identity & Default Behavior
- **Browser-First OS** - boots directly into browser-ready state
- **Default URL** loaded automatically (http://example.com/)
- **Clear purpose** - "Browser-First Operating System"

#### Focus & Persistence
- **Active process tracking** (Browser PID=1 has focus)
- **URL persistence** stored in kernel memory
- **State survives** within boot session
- **Visual feedback** showing active app and current URL

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

When you run AutismOS, you'll experience a **clean, interactive graphical interface** (Step 9):

#### On Screen (VGA Display):

**Boot Screen:**
```
  █████╗ ██╗   ██╗████████╗██╗███████╗███╗   ███╗ ██████╗ ███████╗
 ██╔══██╗██║   ██║╚══██╔══╝██║██╔════╝████╗ ████║██╔═══██╗██╔════╝
 ███████║██║   ██║   ██║   ██║███████╗██╔████╔██║██║   ██║███████╗
 ██╔══██║██║   ██║   ██║   ██║╚════██║██║╚██╔╝██║██║   ██║╚════██║
 ██║  ██║╚██████╔╝   ██║   ██║███████║██║ ╚═╝ ██║╚██████╔╝███████║
 ╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝

                    Browser-First Operating System

  Booting...
```

**Interactive Browser Screen (Step 9):**
```
+----------------------------------------------------------------------------+
| AutismOS - Browser                                                         |
+----------------------------------------------------------------------------+

URL: +----------------------------------------------------------+
     | http://example.com/                                   _  |
     +----------------------------------------------------------+


Press ENTER to navigate
Type to edit URL

+----------------------------------------------------------------------------+
| Content will appear here...                                                |
|                                                                            |
+----------------------------------------------------------------------------+
```

**Key Features:**
- ✅ **Graphical interface** with borders and visual structure
- ✅ **Interactive URL textbox** - type directly, see characters appear
- ✅ **Blinking cursor** shows where you're typing
- ✅ **Enter key** to submit and load pages
- ✅ **In-page updates** - no screen clearing
- ✅ **Visual feedback** - focused elements highlighted
- ✅ **Clean separation** between UI and content areas

#### Behind the Scenes:
- Renderer process (PID=0) fetches and renders pages
- Browser process (PID=1) handles UI and input
- Monitor process (PID=2) tracks system stats
- All debug output goes to **serial port only** (use `-serial stdio` to see it)

**Key Features:**
- ✅ Clean boot with no technical spam
- ✅ Immediate interactivity (keyboard shortcuts work)
- ✅ Clear system identity and purpose
- ✅ Professional look and feel
- ✅ Three processes working seamlessly in background


---

### **Documentation**

- **`STEP6_7_IMPLEMENTATION.md`**: Complete Step 6 & 7 implementation guide
- **`STEP5_IPC.md`**: IPC implementation details
- **`IMPLEMENTATION_SUMMARY.md`**: Feature summary
- **`ARCHITECTURE.md`**: System architecture

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
  - **`kernel/ux/`**: **NEW** User experience layer (ux.c) - Step 8
- **`drivers/`**: Hardware drivers, organized by device type:
  - **`drivers/input/`**: Input devices (keyboard.c with hotkey support, mouse.c, input.c)
  - **`drivers/video/`**: Video driver (video.c with debug_print for serial-only output)
  - **`drivers/storage/`**: Storage devices (disk.c)
  - **`drivers/audio/`**: Audio devices (sound.c)
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

---

## **Step 9: How to Experience the Interactive UI**

The interactive UI is best experienced by running AutismOS in QEMU with a graphical display:

### **Interactive Mode (Recommended)**
```bash
# Build the OS
make

# Run with graphical display
qemu-system-x86_64 -cdrom autismos.iso
```

This opens a window showing:
1. **Boot screen** with AutismOS ASCII art logo
2. **Interactive browser** with URL textbox
3. **Blinking cursor** in the input field
4. **Live typing** - characters appear as you type
5. **Content area** for page display

### **Keyboard Interaction**
When the QEMU window has focus:
- **Type** - Characters appear in the URL textbox
- **Backspace** - Delete characters
- **Enter** - Submit URL and load page
- **Alt+B** - Focus browser (already focused by default)
- **Alt+Q** - Quit

### **Headless Mode (Debug)**
```bash
# Run without display (serial output only)
qemu-system-x86_64 -cdrom autismos.iso -serial stdio -nographic
```

Note: In headless mode, you'll only see kernel debug output. The interactive UI requires a graphical display to fully experience.

### **What You'll Notice**
- ✨ **Immediate feedback** - Type and see characters instantly
- ✨ **Blinking cursor** - Shows system is alive and ready
- ✨ **Visual structure** - Clear UI layout with borders
- ✨ **No screen clears** - Updates happen in-place
- ✨ **Professional feel** - System responds like a real OS

This is the **emotional turning point** where AutismOS transforms from a technical demo into something that feels genuinely usable.

