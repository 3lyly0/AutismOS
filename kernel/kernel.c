#include <stdint.h>
#include "kernel.h"
#include "string.h"
#include "gdt.h"
#include "idt.h"
#include "memory.h"
#include "mouse.h"
#include "video.h"
#include "keyboard.h"
#include "io_ports.h"
#include "disk.h"
#include "sound.h"


void kmain() {
    gdt_init();
    idt_init();
    memory_init();
    keyboard_init();
    // mouse_init();
    clear_screen();
    print("Welcome to AutismOS!\n");
    beep();
    print("Press any key to see it on the screen...\n");



    while (1) {
        char c = kb_getchar();
        if (c != 0) {
            char str[2] = {c, '\0'};
            print(str);
        }
    }
}

