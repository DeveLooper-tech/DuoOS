#include "keyboard.h"
#include "io.h"
#include "shell.h"
#include "gui.h"

#define KBD_DATA_PORT 0x60

static const char scancode_to_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,
    '\\','z','x','c','v','b','n','m',',','.','/',
    0,
    '*',
    0,
    ' ',
    0,
    0,0,0,0,0,0,0,0,0,0,
    0,
    0,
};

void keyboard_handler(void) {
    uint8_t scancode = inb(KBD_DATA_PORT);

    if (scancode & 0x80)
        return;

    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            if (gui_desktop_active()) gui_handle_key(c);
            else shell_handle_key(c);
        }
    }
}
