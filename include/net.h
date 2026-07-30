#ifndef DUOOS_NET_H
#define DUOOS_NET_H

#include <stdint.h>

#define IP4(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

void net_init(uint32_t ip_addr);
void net_poll(void);

#endif
