#include "boot_animation.h"
#include "graphics.h"

static void animation_delay(uint32 milliseconds) {
    volatile uint32 count = milliseconds * 2500;
    for (volatile uint32 i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

static void draw_boot_frame(const char* message, uint32 percent, uint8 accent) {
    graphics_clear_screen(COLOR_BLACK);

    draw_filled_rect(32, 36, 256, 118, COLOR_DARK_GRAY);
    draw_rect(32, 36, 256, 118, COLOR_WHITE);
    draw_filled_rect(32, 36, 256, 22, accent);

    draw_text_scaled(78, 52, "AUTISMOS", COLOR_WHITE, 2);
    draw_text(78, 82, "booting pixel desktop", COLOR_LIGHT_GRAY);
    draw_text(78, 106, message, COLOR_WHITE);
    draw_progress_bar(78, 124, 164, percent, accent);

    graphics_present();
}

void boot_animation_init(void) {
    graphics_clear_screen(COLOR_BLACK);
    graphics_present();
}

void boot_animation_frame_1(void) {
    draw_boot_frame("starting kernel", 18, COLOR_BLUE);
}

void boot_animation_frame_2(void) {
    draw_boot_frame("loading drivers", 38, COLOR_CYAN);
}

void boot_animation_frame_3(void) {
    draw_boot_frame("mapping memory", 58, COLOR_LIGHT_CYAN);
}

void boot_animation_frame_4(void) {
    draw_boot_frame("starting desktop", 82, COLOR_GREEN);
}

void boot_animation_frame_5(void) {
    draw_boot_frame("ready", 100, COLOR_LIGHT_GREEN);
}

void boot_animation_play(void) {
    boot_animation_init();
    boot_animation_frame_1();
    animation_delay(120);
    boot_animation_frame_2();
    animation_delay(120);
    boot_animation_frame_3();
    animation_delay(120);
    boot_animation_frame_4();
    animation_delay(120);
    boot_animation_frame_5();
    animation_delay(180);
}
