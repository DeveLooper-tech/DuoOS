#include "mouse.h"
#include "io.h"
#include "pic.h"
#include "gui.h"

#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64
#define PS2_DATA 0x60

static uint8_t packet[3];
static uint8_t packet_index;

static void wait_write(void) {
    for (uint32_t i = 0; i < 100000; i++)
        if (!(inb(PS2_STATUS) & 0x02)) return;
}

static uint8_t wait_read(void) {
    for (uint32_t i = 0; i < 100000; i++)
        if (inb(PS2_STATUS) & 0x01) return inb(PS2_DATA);
    return 0;
}

static void mouse_command(uint8_t command) {
    wait_write(); outb(PS2_COMMAND, 0xD4);
    wait_write(); outb(PS2_DATA, command);
    (void)wait_read();
}

void mouse_init(void) {
    wait_write(); outb(PS2_COMMAND, 0xA8);
    wait_write(); outb(PS2_COMMAND, 0x20);
    uint8_t status = wait_read() | 0x02;
    wait_write(); outb(PS2_COMMAND, 0x60);
    wait_write(); outb(PS2_DATA, status);
    mouse_command(0xF6);
    mouse_command(0xF4);
    pic_clear_mask(12);
}

void mouse_handler(void) {
    uint8_t value = inb(PS2_DATA);
    if (packet_index == 0 && !(value & 0x08))
        return;
    packet[packet_index++] = value;
    if (packet_index == 3) {
        packet_index = 0;
        if (!(packet[0] & 0xC0))
            gui_mouse_event((int8_t)packet[1], (int8_t)packet[2], packet[0] & 0x07);
    }
}
