#include "gui.h"
#include "terminal.h"
#include "shell.h"
#include "nic.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define ICON_LEFT 3
#define ICON_RIGHT 18
#define ICON_TOP 3
#define ICON_BOTTOM 7

static int16_t mouse_x = 40;
static int16_t mouse_y = 12;
static uint8_t previous_buttons;
static uint8_t desktop_active;

static void put_text(uint8_t x, uint8_t y, const char* text, uint8_t color) {
    while (*text != '\0' && x < SCREEN_WIDTH)
        terminal_put_at(x++, y, *text++, color);
}

static void fill(uint8_t left, uint8_t top, uint8_t right, uint8_t bottom, char c, uint8_t color) {
    for (uint8_t y = top; y <= bottom; y++)
        for (uint8_t x = left; x <= right; x++)
            terminal_put_at(x, y, c, color);
}

static void draw_desktop(void) {
    fill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, ' ', 0x13);
    fill(0, 0, SCREEN_WIDTH - 1, 0, ' ', 0x1F);
    put_text(2, 0, "DuoOS 0.2", 0x1F);
    put_text(15, 0, "Desktop", 0x1F);
    put_text(53, 0, "Network: ", 0x1F);
    put_text(62, 0, nic_name(), 0x1F);

    fill(ICON_LEFT, ICON_TOP, ICON_RIGHT, ICON_BOTTOM, ' ', 0x17);
    fill(ICON_LEFT, ICON_TOP, ICON_RIGHT, ICON_TOP, '=', 0x1F);
    fill(ICON_LEFT, ICON_BOTTOM, ICON_RIGHT, ICON_BOTTOM, '=', 0x1F);
    put_text(ICON_LEFT + 5, ICON_TOP + 1, "[>_]", 0x1F);
    put_text(ICON_LEFT + 4, ICON_TOP + 3, "Terminal", 0x1F);

    put_text(3, 10, "Welcome to DuoOS", 0x1F);
    put_text(3, 12, "Click Terminal or press Enter to open the command line.", 0x1F);
    put_text(3, 13, "Use the desktop command to return here.", 0x1F);

    fill(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, ' ', 0x70);
    put_text(2, SCREEN_HEIGHT - 1, "DuoOS", 0x70);
    put_text(51, SCREEN_HEIGHT - 1, "Enter: open Terminal", 0x70);
    terminal_put_at((uint8_t)mouse_x, (uint8_t)mouse_y, '+', 0x4F);
}

void gui_show_desktop(void) {
    desktop_active = 1;
    previous_buttons = 0;
    draw_desktop();
}

void gui_init(void) {
    mouse_x = 40;
    mouse_y = 12;
    gui_show_desktop();
}

uint8_t gui_desktop_active(void) {
    return desktop_active;
}

void gui_handle_key(char c) {
    if (desktop_active && (c == '\n' || c == 't')) {
        desktop_active = 0;
        terminal_clear();
        terminal_write("DuoOS Terminal Type help to list commands\n");
        shell_init();
    }
}

void gui_mouse_event(int8_t delta_x, int8_t delta_y, uint8_t buttons) {
    if (!desktop_active)
        return;
    mouse_x += delta_x / 4;
    mouse_y -= delta_y / 4;
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x >= SCREEN_WIDTH) mouse_x = SCREEN_WIDTH - 1;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y >= SCREEN_HEIGHT - 1) mouse_y = SCREEN_HEIGHT - 2;

    if ((buttons & 1u) && !(previous_buttons & 1u)
        && mouse_x >= ICON_LEFT && mouse_x <= ICON_RIGHT
        && mouse_y >= ICON_TOP && mouse_y <= ICON_BOTTOM) {
        desktop_active = 0;
        terminal_clear();
        terminal_write("DuoOS Terminal Type help to list commands\n");
        shell_init();
    } else {
        draw_desktop();
    }
    previous_buttons = buttons;
}
