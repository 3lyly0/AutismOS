#ifndef PCI_H
#define PCI_H

#include "types.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_VENDOR_ID      0x00
#define PCI_DEVICE_ID      0x02
#define PCI_COMMAND        0x04
#define PCI_STATUS         0x06
#define PCI_BAR0           0x10
#define PCI_INTERRUPT_LINE 0x3C

#define PCI_CMD_IO         0x01
#define PCI_CMD_MEMORY     0x02
#define PCI_CMD_BUS_MASTER 0x04

typedef struct pci_device {
    uint8 bus;
    uint8 slot;
    uint8 func;
    uint16 vendor_id;
    uint16 device_id;
    uint32 bar0;
} pci_device_t;

void pci_init(void);
uint16 pci_read_word(uint8 bus, uint8 slot, uint8 func, uint8 offset);
uint32 pci_read_long(uint8 bus, uint8 slot, uint8 func, uint8 offset);
void pci_write_word(uint8 bus, uint8 slot, uint8 func, uint8 offset, uint16 value);
int pci_find_device(uint16 vendor_id, uint16 device_id, pci_device_t* dev);

#endif
