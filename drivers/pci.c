#include "pci.h"
#include "io.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) |
                        ((uint32_t)function << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) |
                        ((uint32_t)function << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* out) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t id = pci_config_read32((uint8_t)bus, dev, func, 0x00);
                uint16_t vid = id & 0xFFFF;
                if (vid == 0xFFFF) continue;
                uint16_t did = (id >> 16) & 0xFFFF;
                if (vid == vendor_id && did == device_id) {
                    uint32_t class_reg = pci_config_read32((uint8_t)bus, dev, func, 0x08);
                    out->bus = (uint8_t)bus; out->device = dev; out->function = func;
                    out->vendor_id = vid; out->device_id = did;
                    out->class_code = (class_reg >> 24) & 0xFF;
                    out->subclass   = (class_reg >> 16) & 0xFF;
                    out->prog_if    = (class_reg >> 8) & 0xFF;
                    return 1;
                }
            }
        }
    }
    return 0;
}

int pci_find_device_ids(uint16_t vendor_id, const uint16_t* device_ids,
                        uint16_t count, pci_device_t* out) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t id = pci_config_read32((uint8_t)bus, dev, func, 0x00);
                if ((uint16_t)id != vendor_id)
                    continue;
                uint16_t device_id = (uint16_t)(id >> 16);
                for (uint16_t i = 0; i < count; i++) {
                    if (device_id == device_ids[i]) {
                        uint32_t class_reg = pci_config_read32((uint8_t)bus, dev, func, 0x08);
                        out->bus = (uint8_t)bus;
                        out->device = dev;
                        out->function = func;
                        out->vendor_id = vendor_id;
                        out->device_id = device_id;
                        out->class_code = (uint8_t)(class_reg >> 24);
                        out->subclass = (uint8_t)(class_reg >> 16);
                        out->prog_if = (uint8_t)(class_reg >> 8);
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}
