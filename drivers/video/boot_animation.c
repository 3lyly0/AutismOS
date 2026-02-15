#include "boot_animation.h"
#include "graphics.h"
#include "video.h"
#include "io_ports.h"

// Timing delay function (busy wait)
static void animation_delay(uint32 milliseconds) {
    // Approximate delay using PIT timer reads
    // This is a simple busy-wait loop
    volatile uint32 count = milliseconds * 1000;
    for (volatile uint32 i = 0; i < count; i++) {
        // Busy wait
        asm volatile("nop");
    }
}

void boot_animation_init(void) {
    // Clear screen with black background
    graphics_clear_screen(COLOR_BLACK);
}

void boot_animation_frame_1(void) {
    graphics_clear_screen(COLOR_BLACK);
    
    // Draw initial logo - partial reveal
    draw_text(25, 8, " █████╗ ██╗   ██╗████████╗██╗", COLOR_DARK_GRAY);
    
    // Progress indicator
    draw_text(30, 20, "Initializing...", COLOR_LIGHT_GRAY);
    draw_progress_bar(25, 21, 30, 20, COLOR_CYAN);
}

void boot_animation_frame_2(void) {
    graphics_clear_screen(COLOR_BLACK);
    
    // More logo revealed
    draw_text(24, 7, " █████╗ ██╗   ██╗████████╗██╗███████╗███╗   ███╗", COLOR_LIGHT_GRAY);
    draw_text(24, 8, "██╔══██╗██║   ██║╚══██╔══╝██║██╔════╝████╗ ████║", COLOR_LIGHT_GRAY);
    
    draw_text(30, 20, "Loading drivers...", COLOR_LIGHT_GRAY);
    draw_progress_bar(25, 21, 30, 40, COLOR_CYAN);
}

void boot_animation_frame_3(void) {
    graphics_clear_screen(COLOR_BLACK);
    
    // Even more logo
    draw_text(24, 6, " █████╗ ██╗   ██╗████████╗██╗███████╗███╗   ███╗ ██████╗ ███████╗", COLOR_LIGHT_CYAN);
    draw_text(24, 7, "██╔══██╗██║   ██║╚══██╔══╝██║██╔════╝████╗ ████║██╔═══██╗██╔════╝", COLOR_LIGHT_CYAN);
    draw_text(24, 8, "███████║██║   ██║   ██║   ██║███████╗██╔████╔██║██║   ██║███████╗", COLOR_LIGHT_CYAN);
    
    draw_text(28, 20, "Initializing memory...", COLOR_LIGHT_GRAY);
    draw_progress_bar(25, 21, 30, 60, COLOR_CYAN);
}

void boot_animation_frame_4(void) {
    graphics_clear_screen(COLOR_BLACK);
    
    // Full logo with color
    draw_text(24, 6, " █████╗ ██╗   ██╗████████╗██╗███████╗███╗   ███╗ ██████╗ ███████╗", COLOR_LIGHT_CYAN);
    draw_text(24, 7, "██╔══██╗██║   ██║╚══██╔══╝██║██╔════╝████╗ ████║██╔═══██╗██╔════╝", COLOR_LIGHT_CYAN);
    draw_text(24, 8, "███████║██║   ██║   ██║   ██║███████╗██╔████╔██║██║   ██║███████╗", COLOR_CYAN);
    draw_text(24, 9, "██╔══██║██║   ██║   ██║   ██║╚════██║██║╚██╔╝██║██║   ██║╚════██║", COLOR_CYAN);
    
    draw_text(26, 20, "Starting desktop environment...", COLOR_LIGHT_GRAY);
    draw_progress_bar(25, 21, 30, 80, COLOR_GREEN);
}

void boot_animation_frame_5(void) {
    graphics_clear_screen(COLOR_BLACK);
    
    // Complete logo with full colors
    // Note: X coordinate is 6 (not 24) because frame 5 shows the complete 6-line logo
    // which is wider and needs to be positioned further left to fit on screen
    draw_text(6, 6, " █████╗ ██╗   ██╗████████╗██╗███████╗███╗   ███╗ ██████╗ ███████╗", COLOR_LIGHT_CYAN);
    draw_text(6, 7, "██╔══██╗██║   ██║╚══██╔══╝██║██╔════╝████╗ ████║██╔═══██╗██╔════╝", COLOR_LIGHT_CYAN);
    draw_text(6, 8, "███████║██║   ██║   ██║   ██║███████╗██╔████╔██║██║   ██║███████╗", COLOR_CYAN);
    draw_text(6, 9, "██╔══██║██║   ██║   ██║   ██║╚════██║██║╚██╔╝██║██║   ██║╚════██║", COLOR_CYAN);
    draw_text(6, 10, "██║  ██║╚██████╔╝   ██║   ██║███████║██║ ╚═╝ ██║╚██████╔╝███████║", COLOR_LIGHT_BLUE);
    draw_text(6, 11, "╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝", COLOR_LIGHT_BLUE);
    
    draw_text(20, 13, "Browser-First Operating System", COLOR_YELLOW);
    
    draw_text(30, 20, "Boot complete!", COLOR_LIGHT_GREEN);
    draw_progress_bar(25, 21, 30, 100, COLOR_GREEN);
    
    // Animated dots
    draw_text(32, 22, "Loading", COLOR_LIGHT_GRAY);
    animation_delay(200);
    draw_text(39, 22, ".", COLOR_LIGHT_GRAY);
    animation_delay(200);
    draw_text(40, 22, ".", COLOR_LIGHT_GRAY);
    animation_delay(200);
    draw_text(41, 22, ".", COLOR_LIGHT_GRAY);
}

void boot_animation_play(void) {
    boot_animation_init();
    
    // Play animation sequence with delays
    boot_animation_frame_1();
    animation_delay(400);
    
    boot_animation_frame_2();
    animation_delay(400);
    
    boot_animation_frame_3();
    animation_delay(400);
    
    boot_animation_frame_4();
    animation_delay(400);
    
    boot_animation_frame_5();
    animation_delay(600);
}
