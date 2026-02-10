#include "rtl8139.h"
#include "pci.h"
#include "io_ports.h"
#include "memory.h"
#include "string.h"

#define REG_MAC0       0x00
#define REG_TXSTATUS0  0x10
#define REG_TXADDR0    0x20
#define REG_RXBUF      0x30
#define REG_CMD        0x37
#define REG_CAPR       0x38
#define REG_IMR        0x3C
#define REG_ISR        0x3E
#define REG_TXCONFIG   0x40
#define REG_RXCONFIG   0x44
#define REG_CONFIG1    0x52

#define CMD_RESET      0x10
#define CMD_RX_ENABLE  0x08
#define CMD_TX_ENABLE  0x04

#define RX_ACCEPT_ALL  0x0F
#define RX_BUF_WRAP    0x80

static uint32 io_base = 0;
static uint8  mac_address[6];

static uint8* rx_buffer = 0;
static uint8* tx_buffers[4] = {0};
static uint8  current_tx = 0;
static uint16 rx_offset  = 0;

static void (*receive_callback)(uint8*, uint16) = 0;

/* IO helpers */
static inline uint8  r8(uint16 r) { return inportb(io_base + r); }
static inline uint16 r16(uint16 r){ return inports(io_base + r); }
static inline uint32 r32(uint16 r){ return inportl(io_base + r); }

static inline void w8 (uint16 r, uint8  v){ outportb(io_base + r, v); }
static inline void w16(uint16 r, uint16 v){ outports(io_base + r, v); }
static inline void w32(uint16 r, uint32 v){ outportl(io_base + r, v); }

static uint8 rx_buffer_raw[RTL8139_RX_BUF_SIZE + 16] __attribute__((aligned(16)));


void rtl8139_set_receive_callback(void (*cb)(uint8*, uint16)) {
    receive_callback = cb;
}

void rtl8139_init(void) {
    pci_device_t dev;

    if (pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, &dev) != 0) {
        return;
    }

    io_base = dev.bar0 & ~3;

    uint16 cmd = pci_read_word(dev.bus, dev.slot, dev.func, PCI_COMMAND);
    cmd |= PCI_CMD_IO | PCI_CMD_BUS_MASTER;
    pci_write_word(dev.bus, dev.slot, dev.func, PCI_COMMAND, cmd);

    w8(REG_CONFIG1, 0x00);

    w8(REG_CMD, CMD_RESET);
    while (r8(REG_CMD) & CMD_RESET);

    for (int i = 0; i < 6; i++) {
        mac_address[i] = r8(REG_MAC0 + i);
    }

    rx_buffer = rx_buffer_raw;
    memset(rx_buffer, 0, RTL8139_RX_BUF_SIZE + 16);
    w32(REG_RXBUF, (uint32)rx_buffer);


    for (int i = 0; i < 4; i++) {
        tx_buffers[i] = (uint8*)kmalloc(RTL8139_TX_BUF_SIZE);
    }

    /* RX/TX config */
    w32(REG_RXCONFIG, RX_ACCEPT_ALL | RX_BUF_WRAP);
    w32(REG_TXCONFIG, 0x03000000);

    /* Enable RX/TX */
    w8(REG_CMD, CMD_RX_ENABLE | CMD_TX_ENABLE);

    /* Enable RX interrupt flags (even for polling ISR must update) */
    w16(REG_IMR, 0x0005);   /* RX OK | TX OK */
    w16(REG_ISR, 0xFFFF);   /* Clear pending */
}

int rtl8139_send_packet(const uint8* data, uint16 length) {
    if (!io_base || length > RTL8139_TX_BUF_SIZE) {
        return -1;
    }

    memcpy(tx_buffers[current_tx], data, length);

    w32(REG_TXADDR0   + current_tx * 4, (uint32)tx_buffers[current_tx]);
    w32(REG_TXSTATUS0 + current_tx * 4, length);

    current_tx = (current_tx + 1) & 3;
    return 0;
}

int rtl8139_poll_receive(void) {
    if (!io_base) return 0;

    /* Poll RX buffer directly (no ISR gating) */
    while (!(r8(REG_CMD) & 0x01)) {

        uint16 rx_status = *(uint16*)(rx_buffer + rx_offset);
        uint16 rx_len    = *(uint16*)(rx_buffer + rx_offset + 2);

        if (rx_status & 0x01) {
            uint8* pkt = rx_buffer + rx_offset + 4;
            if (receive_callback && rx_len > 4) {
                receive_callback(pkt, rx_len - 4);
            }
        }

        rx_offset = (rx_offset + rx_len + 4 + 3) & ~3;
        rx_offset %= RTL8139_RX_BUF_SIZE;

        w16(REG_CAPR, rx_offset - 16);
    }

    return 1;
}

uint8* rtl8139_get_mac_address(void) {
    return mac_address;
}
