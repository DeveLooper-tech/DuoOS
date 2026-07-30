#include <stdint.h>
#include "terminal.h"
#include "idt.h"
#include "pic.h"
#include "pmm.h"
#include "nic.h"
#include "net.h"
#include "gui.h"
#include "mouse.h"

void kernel_main(uint64_t multiboot_info_addr) {
    terminal_clear();
    idt_init();

    pic_remap();

    pmm_init(multiboot_info_addr);

    (void)pmm_alloc_frame();

    if (nic_init()) {
        net_init(IP4(10, 0, 2, 15));
    }

    gui_init();
    mouse_init();

    __asm__ volatile ("sti");

    for (;;) {
        net_poll();
    }
}
