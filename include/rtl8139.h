#ifndef DUOOS_RTL8139_H
#define DUOOS_RTL8139_H

#include <stdint.h>

int rtl8139_init(void);
void rtl8139_get_mac(uint8_t out[6]);
void rtl8139_send(const uint8_t* data, uint16_t len);
int rtl8139_poll(uint8_t** out_buf, uint16_t* out_len);

#endif
