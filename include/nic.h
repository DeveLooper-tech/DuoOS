#ifndef DUOOS_NIC_H
#define DUOOS_NIC_H

#include <stdint.h>

int nic_init(void);
const char* nic_name(void);
void nic_get_mac(uint8_t out[6]);
void nic_send(const uint8_t* data, uint16_t len);
int nic_poll(uint8_t** out_buf, uint16_t* out_len);

#endif
