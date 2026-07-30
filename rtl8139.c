#include "rtl8139.h"
#include "io.h"
#include "pci.h"

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139
#define RTL_RX_BUFFER_SIZE 32768

#define REG_IDR0   0x00
#define REG_TSD0   0x10
#define REG_TSAD0  0x20
#define REG_RBSTART 0x30
#define REG_CR     0x37
#define REG_CAPR   0x38
#define REG_IMR    0x3C
#define REG_RCR    0x44

#define CR_RESET 0x10
#define CR_RE    0x08
#define CR_TE    0x04
#define RCR_ACCEPT_ALL 0x0000000F
#define RCR_WRAP 0x00000080

static uint16_t io_base;
static uint8_t mac[6];
static uint16_t rx_offset;
static uint8_t rx_buffer[RTL_RX_BUFFER_SIZE + 16] __attribute__((aligned(4096)));
static uint8_t tx_buffer[2048] __attribute__((aligned(16)));

int rtl8139_init(void) {
    pci_device_t dev;
    if (!pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, &dev))
        return 0;

    uint32_t command = pci_config_read32(dev.bus, dev.device, dev.function, 0x04);
    pci_config_write32(dev.bus, dev.device, dev.function, 0x04, command | 0x0005);
    io_base = (uint16_t)(pci_config_read32(dev.bus, dev.device, dev.function, 0x10) & ~3u);
    if (io_base == 0)
        return 0;

    outb(io_base + REG_CR, CR_RESET);
    for (uint32_t i = 0; i < 100000; i++)
        if (!(inb(io_base + REG_CR) & CR_RESET)) break;
    if (inb(io_base + REG_CR) & CR_RESET)
        return 0;

    for (uint8_t i = 0; i < 6; i++)
        mac[i] = inb(io_base + REG_IDR0 + i);
    outl(io_base + REG_RBSTART, (uint32_t)(uintptr_t)rx_buffer);
    outw(io_base + REG_IMR, 0);
    outl(io_base + REG_RCR, RCR_ACCEPT_ALL | RCR_WRAP);
    outb(io_base + REG_CR, CR_RE | CR_TE);
    rx_offset = 0;
    return 1;
}

void rtl8139_get_mac(uint8_t out[6]) {
    for (uint8_t i = 0; i < 6; i++) out[i] = mac[i];
}

void rtl8139_send(const uint8_t* data, uint16_t len) {
    if (len > sizeof(tx_buffer)) return;
    for (uint16_t i = 0; i < len; i++) tx_buffer[i] = data[i];
    outl(io_base + REG_TSAD0, (uint32_t)(uintptr_t)tx_buffer);
    outl(io_base + REG_TSD0, len);
    for (uint32_t i = 0; i < 1000000; i++)
        if (inl(io_base + REG_TSD0) & (1u << 15)) break;
}

int rtl8139_poll(uint8_t** out_buf, uint16_t* out_len) {
    if (!(inb(io_base + REG_CR) & 0x01))
        return 0;
    uint32_t header = *(uint32_t*)(rx_buffer + rx_offset);
    if (!(header & 0x0001))
        return 0;

    uint16_t received = (uint16_t)(header >> 16);
    uint16_t frame_len = received >= 4 ? (uint16_t)(received - 4) : 0;
    if (frame_len > 1518) frame_len = 0;
    *out_buf = rx_buffer + rx_offset + 4;
    *out_len = frame_len;

    rx_offset = (uint16_t)((rx_offset + received + 4 + 3) & ~3u);
    if (rx_offset >= RTL_RX_BUFFER_SIZE)
        rx_offset = (uint16_t)(rx_offset - RTL_RX_BUFFER_SIZE);
    outw(io_base + REG_CAPR, (uint16_t)(rx_offset - 16));
    return frame_len != 0;
}
