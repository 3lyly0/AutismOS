#include "kernel.h"
#include "string.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "bitmap.h"


void print(const char *str) {
    volatile char *video_memory = (volatile char *)0xB8000;
    static unsigned int index = 0;
    while (*str) {
        video_memory[index * 2] = *str;
        video_memory[index * 2 + 1] = 0x0F;
        index++;
        str++;
    }
}


void kmain() {
    gdt_init();
    idt_init();
    keyboard_init();
    
    while (1) {
        char c = kb_getchar();
        if (c != 0) {
            char str[2] = {c, '\0'};
            print(str);
        }
    }
}