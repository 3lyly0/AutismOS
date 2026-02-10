#include "rtl8139.h"
#include "pci.h"
#include "io_ports.h"
#include "memory.h"
#include "string.h"

#define REG_MAC0       0x00
#define REG_MAR0       0x08
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

#define RX_BUF_WRAP    0x80
#define RX_ACCEPT_ALL  0x0F

static uint32 io_base = 0;
static uint8 mac_address[6];
static uint8* rx_buffer = NULL;
static uint8* tx_buffers[4] = {NULL, NULL, NULL, NULL};
static uint8 current_tx = 0;
static uint16 rx_offset = 0;

static void (*receive_callback)(uint8* packet, uint16 length) = NULL;

static inline uint8 rtl_read8(uint16 reg) {
    return inportb(io_base + reg);
}

static inline uint16 rtl_read16(uint16 reg) {
    return inports(io_base + reg);
}

static inline uint32 rtl_read32(uint16 reg) {
    return inportl(io_base + reg);
}

static inline void rtl_write8(uint16 reg, uint8 val) {
    outportb(io_base + reg, val);
}

static inline void rtl_write16(uint16 reg, uint16 val) {
    outports(io_base + reg, val);
}

static inline void rtl_write32(uint16 reg, uint32 val) {
    outportl(io_base + reg, val);
}

void rtl8139_set_receive_callback(void (*callback)(uint8*, uint16)) {
    receive_callback = callback;
}

void rtl8139_init(void) {
    pci_device_t pci_dev;
    
    if (pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, &pci_dev) != 0) {
        return;
    }
    
    io_base = pci_dev.bar0 & ~3;
    
    uint16 command = pci_read_word(pci_dev.bus, pci_dev.slot, pci_dev.func, PCI_COMMAND);
    command |= PCI_CMD_IO | PCI_CMD_BUS_MASTER;
    pci_write_word(pci_dev.bus, pci_dev.slot, pci_dev.func, PCI_COMMAND, command);
    
    rtl_write8(REG_CONFIG1, 0x00);
    
    rtl_write8(REG_CMD, CMD_RESET);
    while ((rtl_read8(REG_CMD) & CMD_RESET) != 0);
    
    for (int i = 0; i < 6; i++) {
        mac_address[i] = rtl_read8(REG_MAC0 + i);
    }
    
    rx_buffer = (uint8*)kmalloc(RTL8139_RX_BUF_SIZE + 16);
    rtl_write32(REG_RXBUF, (uint32)rx_buffer);
    
    for (int i = 0; i < 4; i++) {
        tx_buffers[i] = (uint8*)kmalloc(RTL8139_TX_BUF_SIZE);
    }
    
    rtl_write32(REG_IMR, 0x0000);
    rtl_write32(REG_ISR, 0xFFFF);
    
    rtl_write32(REG_RXCONFIG, RX_ACCEPT_ALL | RX_BUF_WRAP);
    rtl_write32(REG_TXCONFIG, 0x03000000);
    
    rtl_write8(REG_CMD, CMD_RX_ENABLE | CMD_TX_ENABLE);
}

int rtl8139_send_packet(const uint8* data, uint16 length) {
    if (!io_base || length > RTL8139_TX_BUF_SIZE) {
        return -1;
    }
    
    memcpy(tx_buffers[current_tx], data, length);
    
    rtl_write32(REG_TXADDR0 + (current_tx * 4), (uint32)tx_buffers[current_tx]);
    rtl_write32(REG_TXSTATUS0 + (current_tx * 4), length);
    
    current_tx = (current_tx + 1) % 4;
    
    return 0;
}

int rtl8139_poll_receive(void) {
    if (!io_base) return 0;
    
    uint16 isr = rtl_read16(REG_ISR);
    if (!(isr & 0x01)) return 0;
    
    rtl_write16(REG_ISR, 0x01);
    
    while ((rtl_read8(REG_CMD) & 0x01) == 0) {
        uint16 rx_status = *(uint16*)(rx_buffer + rx_offset);
        uint16 rx_length = *(uint16*)(rx_buffer + rx_offset + 2);
        
        if (rx_status & 0x01) {
            uint8* packet = rx_buffer + rx_offset + 4;
            if (receive_callback) {
                receive_callback(packet, rx_length - 4);
            }
        }
        
        rx_offset = (rx_offset + rx_length + 4 + 3) & ~3;
        rx_offset %= RTL8139_RX_BUF_SIZE;
        
        rtl_write16(REG_CAPR, rx_offset - 16);
    }
    
    return 1;
}

uint8* rtl8139_get_mac_address(void) {
    return mac_address;
}
