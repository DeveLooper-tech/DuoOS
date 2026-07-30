#include "nic.h"
#include "e1000.h"
#include "rtl8139.h"

typedef enum { NIC_NONE, NIC_E1000, NIC_RTL8139 } nic_driver_t;
static nic_driver_t active_driver;

int nic_init(void) {
    if (e1000_init()) { active_driver = NIC_E1000; return 1; }
    if (rtl8139_init()) { active_driver = NIC_RTL8139; return 1; }
    active_driver = NIC_NONE;
    return 0;
}

const char* nic_name(void) {
    if (active_driver == NIC_E1000) return "Intel E1000";
    if (active_driver == NIC_RTL8139) return "Realtek RTL8139";
    return "No supported adapter";
}

void nic_get_mac(uint8_t out[6]) {
    if (active_driver == NIC_E1000) e1000_get_mac(out);
    else if (active_driver == NIC_RTL8139) rtl8139_get_mac(out);
}

void nic_send(const uint8_t* data, uint16_t len) {
    if (active_driver == NIC_E1000) e1000_send(data, len);
    else if (active_driver == NIC_RTL8139) rtl8139_send(data, len);
}

int nic_poll(uint8_t** out_buf, uint16_t* out_len) {
    if (active_driver == NIC_E1000) return e1000_poll(out_buf, out_len);
    if (active_driver == NIC_RTL8139) return rtl8139_poll(out_buf, out_len);
    return 0;
}
