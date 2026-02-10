#include <stdint.h>
#include "kernel.h"
#include "string.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "memory.h"
#include "multiboot.h"
#include "video.h"
#include "graphics.h"
#include "keyboard.h"
#include "io_ports.h"
#include "sound.h"
#include "task.h"
#include "process.h"
#include "syscall.h"
#include "usermode.h"
#include "ipc.h"
#include "shm.h"
#include "input.h"
#include "network.h"
#include "html.h"
#include "layout.h"
#include "ux.h"
#include "rtl8139.h"

/* =========================
   Globals
   ========================= */

volatile uint64 g_timer_ticks = 0;
volatile uint32 g_framebuffer_shm_id = 0;
volatile ping_response_t g_ping_response;

volatile uint32 browser_counter = 0;
volatile uint32 renderer_counter = 0;
volatile uint32 browser_page_loaded = 0;

/* PIDs */
static uint32 pid_renderer = 0;
static uint32 pid_browser  = 0;

/* =========================
   Timer IRQ
   ========================= */

void timer_interrupt_handler(REGISTERS* r) {
    (void)r;
    g_timer_ticks++;
    ux_update_caret();
}

/* =========================
   Yield (guaranteed)
   ========================= */

static inline void kernel_yield(void) {
    asm volatile("int $0x20");
}

/* =========================
   Syscall helpers
   ========================= */

static inline int sys_send_msg(uint32 pid, uint32 type, uint32 d1, uint32 d2) {
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type  = type;
    msg.data1 = d1;
    msg.data2 = d2;

    int ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_SEND), "b"(pid), "c"(&msg));
    return ret;
}

static inline int sys_poll_msg(message_t* msg) {
    int ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_POLL), "b"(msg));
    return ret;
}

static inline uint32 sys_shm_create(uint32 size) {
    uint32 ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_SHM_CREATE), "b"(size));
    return ret;
}

static inline int sys_shm_map(uint32 id, void** out) {
    int ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_SHM_MAP), "b"(id), "c"(out));
    return ret;
}

/* =========================
   NETWORK PROCESS
   ========================= */

void net_process(void) {
    for (;;) {
        rtl8139_poll_receive();
        kernel_yield();
    }
}

/* =========================
   RENDERER PROCESS
   ========================= */

void renderer_process(void) {
    const uint32 FB_W = 320;
    const uint32 FB_H = 200;
    const uint32 FB_SIZE = FB_W * FB_H * sizeof(uint32);

    uint32 shm_id = sys_shm_create(FB_SIZE);
    g_framebuffer_shm_id = shm_id;

    uint32* pixels = NULL;
    sys_shm_map(shm_id, (void**)&pixels);

    for (;;) {
        renderer_counter++;

        message_t msg;
        if (sys_poll_msg(&msg) == 0 && msg.type == 6 /* URL REQUEST */) {

            const char* ip = ux_get_last_ip();

            ping_response_t resp;
            memset(&resp, 0, sizeof(resp));
            ping_ip(ip, &resp);
            g_ping_response = resp;

            for (uint32 i = 0; i < FB_W * FB_H; i++)
                pixels[i] = 0xFF000000;

            sys_send_msg(msg.sender_pid, 5 /* FRAME READY */, 0, shm_id);
        }

        kernel_yield();
    }
}

/* =========================
   BROWSER PROCESS
   ========================= */

void browser_process(void) {
    char last_ip[64] = {0};

    while (!g_framebuffer_shm_id)
        kernel_yield();

    for (;;) {
        browser_counter++;

        const char* ip = ux_get_last_ip();
        if (ip && ip[0] && strcmp(ip, last_ip) != 0) {
            strncpy(last_ip, ip, sizeof(last_ip) - 1);
            browser_page_loaded = 0;
            sys_send_msg(pid_renderer, 6 /* URL REQUEST */, 0, 0);
        }

        message_t msg;
        if (sys_poll_msg(&msg) == 0 && msg.type == 5 /* FRAME READY */) {

            graphics_clear_region(2, 14, 76, 9, COLOR_BLACK);

            if (g_ping_response.success)
                draw_text(3, 15, "Ping successful", COLOR_LIGHT_GREEN);
            else
                draw_text(3, 15, "Ping failed", COLOR_LIGHT_RED);

            draw_text(3, 17, "Message:", COLOR_WHITE);
            draw_text(12, 17,
                      (const char*)g_ping_response.message,
                      COLOR_LIGHT_GRAY);

            browser_page_loaded = 1;
        }

        kernel_yield();
    }
}

/* =========================
   KERNEL MAIN PROCESS
   ========================= */

void kernel_main_process(void) {
    for (;;)
        kernel_yield();
}

/* =========================
   KMAIN
   ========================= */

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
    network_init();
    html_init();
    layout_init();

    task_init();
    process_init();

    process_create(net_process, 0);
    pid_renderer = process_create(renderer_process, 0)->pid;
    pid_browser  = process_create(browser_process, 0)->pid;
    process_create(kernel_main_process, 0);

    beep();
    ux_finish_boot();

    asm volatile("sti");

    for (;;)
        asm volatile("hlt");
}
