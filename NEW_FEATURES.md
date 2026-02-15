# New Features: Boot Animation & Smooth Mouse Control

This document describes the newly added features to AutismOS: the animated boot sequence and the smooth mouse control system.

## Boot Animation

### Overview
The boot animation system provides a professional, visually appealing boot sequence that replaces the simple static boot screen.

### Implementation
- **Location**: `drivers/video/boot_animation.c`
- **Header**: `include/boot_animation.h`
- **Integration**: Called from `ux_show_boot_screen()` in `kernel/ux/ux.c`

### Animation Sequence

The boot animation consists of 5 frames, each building upon the previous:

#### Frame 1 (20% complete)
- Partial logo reveal
- Message: "Initializing..."
- Progress: 20%

#### Frame 2 (40% complete)
- More logo revealed
- Message: "Loading drivers..."
- Progress: 40%

#### Frame 3 (60% complete)
- Even more logo with color
- Message: "Initializing memory..."
- Progress: 60%

#### Frame 4 (80% complete)
- Full logo with enhanced colors
- Message: "Starting desktop environment..."
- Progress: 80%

#### Frame 5 (100% complete)
- Complete logo with full ASCII art
- Message: "Boot complete!"
- Progress: 100%
- Animated loading dots

### Timing
Each frame displays for approximately 400-600ms, creating a smooth animation flow.

### Customization
To modify the animation:
1. Edit `drivers/video/boot_animation.c`
2. Update individual frame functions (`boot_animation_frame_X()`)
3. Adjust delay values in `boot_animation_play()`
4. Rebuild the OS with `make`

---

## Smooth Mouse Control

### Overview
The smooth mouse control system is an **alternative** mouse driver that provides superior cursor movement compared to the original PS/2 mouse driver. Both drivers coexist, and the smooth driver is enabled by default.

### Implementation
- **Location**: `drivers/input/mouse_smooth.c`
- **Header**: `include/mouse_smooth.h`
- **Integration**: Initialized in `kmain()` in `kernel/core/kernel.c`

### Key Features

#### 1. Velocity-Based Smoothing
Instead of directly mapping mouse movement to cursor position, the smooth driver uses velocity:
- Raw mouse input is converted to velocity
- Velocity is smoothed using exponential smoothing (30% new, 70% old)
- Velocity is applied to position
- Results in fluid, natural cursor movement

#### 2. Acceleration
Fast mouse movements receive acceleration for better control:
- Slow movements: 1x speed (precise control)
- Fast movements: Up to 2.5x speed (quick navigation)
- Threshold: 10 units of movement
- Acceleration factor scales with speed

#### 3. Configurable Sensitivity
Sensitivity can be adjusted from 1 (slowest) to 10 (fastest):
```c
mouse_smooth_set_sensitivity(7);  // Set to 7 out of 10
```
- Default: 5 (medium sensitivity)
- Higher values = faster cursor movement
- Lower values = slower, more precise control

#### 4. Momentum Physics
The cursor has momentum and smoothly decelerates:
- Prevents jittery movement
- Feels natural and responsive
- Velocity resets to zero at screen edges

### API Reference

```c
// Initialize the smooth mouse driver
void mouse_smooth_init(void);

// Enable smooth mouse mode
void mouse_smooth_enable(void);

// Disable smooth mouse mode (reverts to regular mouse)
void mouse_smooth_disable(void);

// Check if smooth mode is enabled
uint8 mouse_smooth_is_enabled(void);

// Set sensitivity (1-10, default 5)
void mouse_smooth_set_sensitivity(uint8 level);

// Get cursor position
int mouse_smooth_get_x(void);
int mouse_smooth_get_y(void);

// Get button state
uint8 mouse_smooth_get_buttons(void);
```

### Usage Example

The smooth mouse is initialized and enabled by default in the kernel:

```c
// In kernel initialization
mouse_init();         // Initialize original mouse
mouse_smooth_init();  // Initialize smooth mouse (enabled by default)
```

To switch between modes at runtime (future enhancement):

```c
// Disable smooth mode
mouse_smooth_disable();

// Enable smooth mode
mouse_smooth_enable();

// Adjust sensitivity
mouse_smooth_set_sensitivity(8);  // Faster
```

### Technical Details

#### Smoothing Algorithm
```c
// Exponential smoothing factor
float smoothing = 0.3f;

// Apply smoothing
g_velocity_x = g_velocity_x * (1.0f - smoothing) + raw_vel_x * smoothing;
g_velocity_y = g_velocity_y * (1.0f - smoothing) + raw_vel_y * smoothing;
```

#### Acceleration Algorithm
```c
// Calculate total speed
float speed = abs(raw_vel_x) + abs(raw_vel_y);

// Apply acceleration for fast movements
if (speed > 10.0f) {
    accel_factor = 1.0f + (speed - 10.0f) * 0.05f;
    if (accel_factor > 2.5f) accel_factor = 2.5f;
}
```

### Why Two Mouse Drivers?

The requirements specified **not to modify the existing mouse driver**, so we created a new alternative:

1. **Original Driver** (`mouse.c`):
   - Simple, direct mapping
   - Divide-by-3 sensitivity reduction
   - Basic accumulator for fractional movement
   - Reliable but can feel "jumpy"

2. **Smooth Driver** (`mouse_smooth.c`):
   - Velocity-based smoothing
   - Acceleration support
   - Momentum physics
   - Configurable sensitivity
   - Feels more responsive and natural

**Note on IRQ Handling**: Both drivers register handlers on IRQ12 (PS/2 mouse interrupt). Since only one handler can be active at a time, the last registered handler (smooth mouse) takes precedence. When smooth mode is enabled, it handles all mouse events. When disabled, the handler is still registered but inactive, allowing future enhancements to switch between drivers.

### Performance
- Minimal overhead: IRQ handler is optimized
- No heap allocations: All state is static
- Float operations: Simple enough for real-time use
- Compatible with existing desktop system

---

## Building and Testing

### Build
```bash
make
```

### Run in QEMU
```bash
qemu-system-x86_64 -cdrom autismos.iso
```

### What You'll See

1. **Boot Animation**: A professional animated sequence showing the AutismOS logo progressively revealing with progress indicators

2. **Desktop**: After boot, you'll see the desktop environment with windows

3. **Smooth Mouse**: Move the mouse and notice:
   - Fluid cursor movement
   - Natural acceleration on fast movements
   - Smooth deceleration

---

## Future Enhancements

### Boot Animation
- [ ] Add sound effects during boot
- [ ] Customizable themes
- [ ] Error state animations
- [ ] Shutdown animation

### Smooth Mouse
- [ ] GUI settings panel for sensitivity
- [ ] Mouse acceleration curves
- [ ] Left-handed mode
- [ ] Double-click detection
- [ ] Gesture support

---

## Troubleshooting

### Boot Animation Not Showing
- Check that `boot_animation.c` is compiled: Look for `boot_animation.o` in `build/` directory
- Verify `ux_show_boot_screen()` calls `boot_animation_play()`
- Check QEMU display settings (VGA text mode required)

### Mouse Not Smooth
- Verify smooth mouse is enabled: `mouse_smooth_is_enabled()`
- Check IRQ 12 is properly configured
- Try adjusting sensitivity: `mouse_smooth_set_sensitivity()`
- Ensure both mouse drivers are initialized

### Build Errors
- Make sure all new files are in `OBJECTS` list in Makefile
- Check that header files are in `include/` directory
- Verify proper `#include` statements

---

## Credits

Implemented as requested in issue: "تحكم الماوس سيء جدا جدا جدا شوف طريقة تانيه مختلفه متعدلش الموجودة وضيف بوت انميشن 🙂🙂"

Translation: "Mouse control is very very very bad, look for a different method, don't modify the existing one, and add boot animation 🙂🙂"
