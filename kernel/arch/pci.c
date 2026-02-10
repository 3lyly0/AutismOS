#include "pci.h"
#include "io_ports.h"

void pci_init(void) {
}

static uint32 pci_config_address(uint8 bus, uint8 slot, uint8 func, uint8 offset) {
    return (uint32)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
}

uint16 pci_read_word(uint8 bus, uint8 slot, uint8 func, uint8 offset) {
    uint32 address = pci_config_address(bus, slot, func, offset);
    outportl(PCI_CONFIG_ADDRESS, address);
    return (uint16)((inportl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
}

uint32 pci_read_long(uint8 bus, uint8 slot, uint8 func, uint8 offset) {
    uint32 address = pci_config_address(bus, slot, func, offset);
    outportl(PCI_CONFIG_ADDRESS, address);
    return inportl(PCI_CONFIG_DATA);
}

void pci_write_word(uint8 bus, uint8 slot, uint8 func, uint8 offset, uint16 value) {
    uint32 address = pci_config_address(bus, slot, func, offset);
    outportl(PCI_CONFIG_ADDRESS, address);
    uint32 data = inportl(PCI_CONFIG_DATA);
    data &= ~(0xFFFF << ((offset & 2) * 8));
    data |= (value << ((offset & 2) * 8));
    outportl(PCI_CONFIG_DATA, data);
}

int pci_find_device(uint16 vendor_id, uint16 device_id, pci_device_t* dev) {
    for (uint8 bus = 0; bus < 256; bus++) {
        for (uint8 slot = 0; slot < 32; slot++) {
            uint16 vendor = pci_read_word(bus, slot, 0, PCI_VENDOR_ID);
            if (vendor == 0xFFFF) continue;
            
            uint16 device = pci_read_word(bus, slot, 0, PCI_DEVICE_ID);
            if (vendor == vendor_id && device == device_id) {
                dev->bus = bus;
                dev->slot = slot;
                dev->func = 0;
                dev->vendor_id = vendor;
                dev->device_id = device;
                dev->bar0 = pci_read_long(bus, slot, 0, PCI_BAR0);
                return 0;
            }
        }
    }
    return -1;
}
