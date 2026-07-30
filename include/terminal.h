#ifndef DUOOS_TERMINAL_H
#define DUOOS_TERMINAL_H

#include <stdint.h>

void terminal_clear(void);
void terminal_putchar(char c);
void terminal_write(const char* str);
void terminal_write_hex(uint64_t value);
void terminal_write_dec(uint64_t value);
void terminal_put_at(uint8_t x, uint8_t y, char c, uint8_t color);

#endif
