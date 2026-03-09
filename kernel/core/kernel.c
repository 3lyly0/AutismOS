#include <stdint.h>
#include "kernel.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "memory.h"
#include "multiboot.h"
#include "video.h"
#include "keyboard.h"
#include "sound.h"
#include "task.h"
#include "process.h"
#include "syscall.h"
#include "usermode.h"
#include "shm.h"
#include "input.h"
#include "network.h"
#include "html.h"
#include "layout.h"
#include "ux.h"
#include "desktop.h"
#include "notepad.h"
#include "mouse.h"
#include "mouse_smooth.h"

volatile uint64 g_timer_ticks = 0;



void kernel_panic(const char* message) {
    asm volatile("cli");

    print("\n\n*** KERNEL PANIC ***\n");
    print(message);
    print("\nSystem halted.\n");

    for (;;)
        asm volatile("hlt");
}



/* =========================
   Timer IRQ
   ========================= */

void timer_interrupt_handler(REGISTERS* r) {
    (void)r;
    g_timer_ticks++;
}

void kmain(uint32 magic, multiboot_info_t* mbi) {
    ux_init();
    ux_show_boot_screen();

    gdt_init();
    idt_init();
    isr_register_interrupt_handler(
        IRQ_BASE + IRQ0_TIMER,
        timer_interrupt_handler
    );

    if (magic != MULTIBOOT_MAGIC)
        kernel_panic("Invalid multiboot magic");

    memory_init(mbi);
    paging_init();
    paging_enable();

    keyboard_init();
    syscall_init();
    usermode_init();

    uint32 stack;
    asm volatile("mov %%esp, %0" : "=r"(stack));
    tss_init(0x10, stack);

    shm_init();
    input_init();
    mouse_init();  // Initialize PS/2 mouse (original driver)
    mouse_smooth_init();  // Initialize smooth mouse (new alternative driver)
    network_init();
    html_init();
    layout_init();

    task_init();
    process_init();

    // Run the current desktop shell directly from the kernel main loop.
    desktop_init();
    desktop_activate();
    notepad_create();

    beep();
    ux_finish_boot();

    asm volatile("sti");

    // Run desktop in main loop
    for (;;) {
        desktop_t* ds = desktop_get_state();
        desktop_handle_mouse(ds->mouse_x, ds->mouse_y, ds->mouse_buttons);
        desktop_draw();
        asm volatile("hlt");
    }
}
