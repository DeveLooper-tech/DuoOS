#include <stdint.h>
#include "terminal.h"
#include "idt.h"
#include "pic.h"
#include "pmm.h"
#include "e1000.h"
#include "net.h"
#include "shell.h"
#include "gui.h"
#include "mouse.h"

void kernel_main(uint64_t multiboot_info_addr) {
    terminal_clear();
    terminal_write("DuoOS v0.3\n");
    terminal_write("64 bit kernel\n");

    idt_init();

    pic_remap();

    pmm_init(multiboot_info_addr);

    (void)pmm_alloc_frame();

    if (e1000_init()) {
        net_init(IP4(10, 0, 2, 15));
    }

    gui_init();
    mouse_init();

    __asm__ volatile ("sti");

    for (;;) {
        net_poll();
    }
}
