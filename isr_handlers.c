#include "idt.h"
#include "terminal.h"
#include "pic.h"
#include "keyboard.h"
#include "mouse.h"

static const char* exception_names[32] = {
    "Division by zero", "Debug", "Non-maskable interrupt", "Breakpoint",
    "Overflow", "Bound range exceeded", "Invalid opcode", "Device not available",
    "Double fault", "Coprocessor segment overrun", "Invalid TSS", "Segment not present",
    "Stack-segment fault", "General protection fault", "Page fault", "Reserved",
    "x87 floating-point exception", "Alignment check", "Machine check", "SIMD floating-point exception",
    "Virtualization exception", "Control protection exception", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor injection exception", "VMM communication exception", "Security exception", "Reserved"
};

/* CPU kivetel (0-31) kezelese - egyelore csak kiirjuk es leallunk */
void isr_handler(registers_t* regs) {
    terminal_write("\n[KIVETEL] ");
    if (regs->int_no < 32)
        terminal_write(exception_names[regs->int_no]);
    terminal_write(" (int_no=");
    terminal_write_dec(regs->int_no);
    terminal_write(", err_code=");
    terminal_write_hex(regs->err_code);
    terminal_write(", rip=");
    terminal_write_hex(regs->rip);
    terminal_write(")\n");
    terminal_write("A rendszer leallt.\n");

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

void irq_handler(registers_t* regs) {
    switch (regs->int_no) {
        case 32:
            break;
        case 33:
            keyboard_handler();
            break;
        case 44:
            mouse_handler();
            break;
        default:
            break;
    }

    pic_send_eoi((uint8_t)(regs->int_no - 32));
}
