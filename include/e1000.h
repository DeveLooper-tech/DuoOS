#ifndef DUOOS_E1000_H
#define DUOOS_E1000_H

#include <stdint.h>

int  e1000_init(void);
void e1000_get_mac(uint8_t out[6]);
void e1000_send(const uint8_t* data, uint16_t len);
int  e1000_poll(uint8_t** out_buf, uint16_t* out_len);

#endif
