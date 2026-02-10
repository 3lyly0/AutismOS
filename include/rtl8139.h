#ifndef RTL8139_H
#define RTL8139_H

#include "types.h"

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

#define RTL8139_RX_BUF_SIZE 8192
#define RTL8139_TX_BUF_SIZE 1536

void rtl8139_init(void);
int rtl8139_send_packet(const uint8* data, uint16 length);
int rtl8139_poll_receive(void);
uint8* rtl8139_get_mac_address(void);
void rtl8139_set_receive_callback(void (*callback)(uint8*, uint16));

#endif
