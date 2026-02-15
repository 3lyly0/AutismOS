# Testing Guide for Boot Animation & Smooth Mouse

## Quick Test

### Build
```bash
make
```

Expected output: `autismos.iso` created successfully (approximately 12MB)

### Run in QEMU (with graphical display)
```bash
qemu-system-x86_64 -cdrom autismos.iso
```

## What to Look For

### Boot Animation
When the OS starts, you should see:

1. **Frame 1** (appears immediately):
   - Partial AutismOS logo in dark gray
   - "Initializing..." message
   - Progress bar at 20%

2. **Frame 2** (~400ms later):
   - More of the logo revealed in light gray
   - "Loading drivers..." message
   - Progress bar at 40%

3. **Frame 3** (~800ms from start):
   - Even more logo with cyan colors
   - "Initializing memory..." message
   - Progress bar at 60%

4. **Frame 4** (~1200ms from start):
   - Full logo with enhanced colors
   - "Starting desktop environment..." message
   - Progress bar at 80%

5. **Frame 5** (~1600ms from start):
   - Complete 6-line ASCII art logo
   - "Browser-First Operating System" subtitle
   - "Boot complete!" message
   - Progress bar at 100%
   - Animated loading dots

Total animation time: ~2.2 seconds

### Desktop with Smooth Mouse

After the boot animation completes, the desktop environment loads.

**Testing Mouse Movement:**

1. **Move the mouse slowly**:
   - Cursor should move smoothly and precisely
   - No jerkiness or stuttering
   - Good for precise clicking on small targets

2. **Move the mouse quickly**:
   - Cursor should accelerate for faster navigation
   - Up to 2.5x acceleration on fast movements
   - Smooth deceleration when you stop

3. **Test edge behavior**:
   - Move cursor to screen edges
   - Should stop at boundaries (0, 0) to (79, 24)
   - No overflow or wrapping

4. **Test sensitivity** (if you modify the code):
   ```c
   // In kernel initialization, add:
   mouse_smooth_set_sensitivity(8);  // Faster (1-10 scale)
   ```

## Expected Behavior

### Boot Animation
- ✅ Smooth progressive reveal
- ✅ No screen flickering
- ✅ Progress bars fill correctly
- ✅ Colors transition properly
- ✅ All text is readable

### Smooth Mouse
- ✅ Fluid cursor movement
- ✅ Natural acceleration/deceleration
- ✅ No cursor jumping
- ✅ Works with desktop windows
- ✅ Click detection works correctly

## Common Issues

### Boot Animation Not Visible
- **Problem**: Animation goes by too fast or not visible
- **Solution**: Check QEMU settings, ensure VGA text mode is active
- **Workaround**: Increase delay values in `boot_animation.c`

### Mouse Not Moving
- **Problem**: Cursor doesn't move at all
- **Check**: Ensure both mouse_init() and mouse_smooth_init() are called
- **Check**: Verify IRQ12 is enabled
- **Solution**: Check QEMU mouse emulation settings

### Mouse Too Fast/Slow
- **Problem**: Cursor moves too quickly or slowly
- **Solution**: Adjust sensitivity:
  ```c
  mouse_smooth_set_sensitivity(3);  // Slower (1-10)
  mouse_smooth_set_sensitivity(8);  // Faster (1-10)
  ```

### Mouse Feels Laggy
- **Problem**: Delay between mouse movement and cursor update
- **Explanation**: This is the smoothing algorithm working
- **Solution**: Reduce smoothing factor in `mouse_smooth.c`:
  ```c
  float smoothing = 0.3f;  // Default
  float smoothing = 0.5f;  // Less smooth, more responsive
  ```

## Performance Notes

### Boot Animation
- No performance impact after boot completes
- Uses standard graphics functions
- Minimal memory footprint

### Smooth Mouse
- Very low CPU overhead (IRQ handler only)
- No dynamic memory allocation
- Float operations are simple and fast
- Desktop redraws are optimized

## Comparing with Original Mouse

To compare smooth mouse vs original:

1. **Current setup**: Smooth mouse is active by default
2. **To test original**: Comment out `mouse_smooth_init()` in kernel.c
3. **Rebuild and test**
4. **You should notice**:
   - Original: More direct but can feel jumpy
   - Smooth: More fluid but with slight smoothing delay

## Advanced Testing

### Sensitivity Sweep
Test different sensitivity levels:

```c
// Add to kernel initialization
for (int i = 1; i <= 10; i++) {
    mouse_smooth_set_sensitivity(i);
    // Move mouse and observe
}
```

### Animation Frame Timing
Adjust delays in `boot_animation.c`:

```c
// In boot_animation_play()
animation_delay(200);  // Faster
animation_delay(800);  // Slower
```

## Success Criteria

### Boot Animation ✓
- [x] All 5 frames display in sequence
- [x] Progress bars show correct percentages
- [x] Colors are vibrant and readable
- [x] Timing feels smooth (not too fast or slow)
- [x] Transitions to desktop properly

### Smooth Mouse ✓
- [x] Cursor moves fluidly
- [x] Acceleration works on fast movements
- [x] No cursor disappearance
- [x] Window interactions work correctly
- [x] No crashes or hangs

## Cleanup

After testing, if you want to remove the test ISO:
```bash
rm autismos.iso
```

To rebuild:
```bash
make clean  # Not implemented, use: rm -rf build
make
```

---

**Note**: This is a text-mode OS, so graphics are rendered using VGA text mode (80x25 characters). The boot animation and smooth mouse work within these constraints.
