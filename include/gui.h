#ifndef DUOOS_GUI_H
#define DUOOS_GUI_H

#include <stdint.h>

void gui_init(void);
void gui_handle_key(char c);
void gui_mouse_event(int8_t delta_x, int8_t delta_y, uint8_t buttons);
void gui_show_desktop(void);
uint8_t gui_desktop_active(void);

#endif
