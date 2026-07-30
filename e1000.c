#include "e1000.h"
#include "pci.h"
#include "pmm.h"

#define REG_CTRL       0x0000
#define REG_RCTRL      0x0100
#define REG_TCTRL      0x0400
#define REG_TIPG       0x0410
#define REG_RXDESCLO   0x2800
#define REG_RXDESCHI   0x2804
#define REG_RXDESCLEN  0x2808
#define REG_RXDESCHEAD 0x2810
#define REG_RXDESCTAIL 0x2818
#define REG_TXDESCLO   0x3800
#define REG_TXDESCHI   0x3804
#define REG_TXDESCLEN  0x3808
#define REG_TXDESCHEAD 0x3810
#define REG_TXDESCTAIL 0x3818
#define REG_RAL        0x5400
#define REG_RAH        0x5404

#define RCTL_EN      (1 << 1)
#define RCTL_UPE     (1 << 3)
#define RCTL_MPE     (1 << 4)
#define RCTL_BAM     (1 << 15)
#define RCTL_SECRC   (1 << 26)

#define TCTL_EN      (1 << 1)
#define TCTL_PSP     (1 << 3)

#define NUM_RX_DESC 8
#define NUM_TX_DESC 8

typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} e1000_rx_desc_t;

typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} e1000_tx_desc_t;

static volatile uint32_t* mmio_base;
static e1000_rx_desc_t* rx_descs;
static e1000_tx_desc_t* tx_descs;
static uint8_t* rx_buffers[NUM_RX_DESC];
static uint16_t rx_tail = 0;
static uint16_t tx_tail = 0;
static uint8_t mac[6];

static inline void reg_write(uint32_t offset, uint32_t value) { mmio_base[offset / 4] = value; }
static inline uint32_t reg_read(uint32_t offset) { return mmio_base[offset / 4]; }

static void read_mac(void) {
    uint32_t ral = reg_read(REG_RAL);
    uint32_t rah = reg_read(REG_RAH);
    mac[0] = ral & 0xFF; mac[1] = (ral >> 8) & 0xFF; mac[2] = (ral >> 16) & 0xFF; mac[3] = (ral >> 24) & 0xFF;
    mac[4] = rah & 0xFF; mac[5] = (rah >> 8) & 0xFF;
}

int e1000_init(void) {
    pci_device_t dev;
    static const uint16_t supported_ids[] = {
        0x1004, 0x100E, 0x100F, 0x1015, 0x1016, 0x1017, 0x101E, 0x1078
    };
    if (!pci_find_device_ids(0x8086, supported_ids,
                             sizeof(supported_ids) / sizeof(supported_ids[0]), &dev)) {
        return 0;
    }

    uint32_t bar0 = pci_config_read32(dev.bus, dev.device, dev.function, 0x10);
    uint64_t mmio_phys = bar0 & ~0xFu;
    mmio_base = (volatile uint32_t*)(uintptr_t)mmio_phys;

    uint32_t command = pci_config_read32(dev.bus, dev.device, dev.function, 0x04);
    command |= (1 << 2) | (1 << 1); /* bus master + memory space */
    pci_config_write32(dev.bus, dev.device, dev.function, 0x04, command);

    read_mac();

    uint64_t rx_phys = pmm_alloc_frame();
    rx_descs = (e1000_rx_desc_t*)(uintptr_t)rx_phys;
    for (int i = 0; i < NUM_RX_DESC; i++) {
        uint64_t buf_phys = pmm_alloc_frame();
        rx_buffers[i] = (uint8_t*)(uintptr_t)buf_phys;
        rx_descs[i].addr = buf_phys;
        rx_descs[i].status = 0;
    }
    reg_write(REG_RXDESCLO, (uint32_t)rx_phys);
    reg_write(REG_RXDESCHI, 0);
    reg_write(REG_RXDESCLEN, NUM_RX_DESC * sizeof(e1000_rx_desc_t));
    reg_write(REG_RXDESCHEAD, 0);
    reg_write(REG_RXDESCTAIL, NUM_RX_DESC - 1);
    reg_write(REG_RCTRL, RCTL_EN | RCTL_UPE | RCTL_MPE | RCTL_BAM | RCTL_SECRC);

    uint64_t tx_phys = pmm_alloc_frame();
    tx_descs = (e1000_tx_desc_t*)(uintptr_t)tx_phys;
    for (int i = 0; i < NUM_TX_DESC; i++) {
        tx_descs[i].addr = 0;
        tx_descs[i].status = 1; /* DD = kesz allapot */
    }
    reg_write(REG_TXDESCLO, (uint32_t)tx_phys);
    reg_write(REG_TXDESCHI, 0);
    reg_write(REG_TXDESCLEN, NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    reg_write(REG_TXDESCHEAD, 0);
    reg_write(REG_TXDESCTAIL, 0);
    reg_write(REG_TCTRL, TCTL_EN | TCTL_PSP | (15 << 4) | (64 << 12));
    reg_write(REG_TIPG, 0x0060200A);

    return 1;
}

void e1000_get_mac(uint8_t out[6]) {
    for (int i = 0; i < 6; i++) out[i] = mac[i];
}

void e1000_send(const uint8_t* data, uint16_t len) {
    tx_descs[tx_tail].addr   = (uint64_t)(uintptr_t)data;
    tx_descs[tx_tail].length = len;
    tx_descs[tx_tail].cmd    = (1 << 0) | (1 << 1) | (1 << 3); /* EOP | IFCS | RS */
    tx_descs[tx_tail].status = 0;

    uint16_t old_tail = tx_tail;
    tx_tail = (tx_tail + 1) % NUM_TX_DESC;
    reg_write(REG_TXDESCTAIL, tx_tail);

    while (!(tx_descs[old_tail].status & 0x1)) { }
}

int e1000_poll(uint8_t** out_buf, uint16_t* out_len) {
    if (!(rx_descs[rx_tail].status & 0x1))
        return 0;

    *out_buf = rx_buffers[rx_tail];
    *out_len = rx_descs[rx_tail].length;
    rx_descs[rx_tail].status = 0;
    reg_write(REG_RXDESCTAIL, rx_tail);
    rx_tail = (rx_tail + 1) % NUM_RX_DESC;
    return 1;
}
