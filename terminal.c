#include "terminal.h"
#include <stddef.h>

static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;
static const size_t VGA_WIDTH  = 80;
static const size_t VGA_HEIGHT = 25;

static size_t  term_row = 0;
static size_t  term_col = 0;
static uint8_t term_color = 0x0A; /* zold szoveg, fekete hatter */

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)(uint8_t)c | ((uint16_t)color << 8);
}

void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', term_color);
    term_row = 0;
    term_col = 0;
}

static void terminal_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];

    for (size_t x = 0; x < VGA_WIDTH; x++)
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', term_color);

    term_row = VGA_HEIGHT - 1;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\b') {
        if (term_col > 0) {
            term_col--;
        } else if (term_row > 0) {
            term_row--;
            term_col = VGA_WIDTH - 1;
        }
        VGA_MEMORY[term_row * VGA_WIDTH + term_col] = vga_entry(' ', term_color);
    } else {
        VGA_MEMORY[term_row * VGA_WIDTH + term_col] = vga_entry(c, term_color);
        if (++term_col == VGA_WIDTH) {
            term_col = 0;
            term_row++;
        }
    }
    if (term_row >= VGA_HEIGHT)
        terminal_scroll();
}

void terminal_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++)
        terminal_putchar(str[i]);
}

void terminal_write_hex(uint64_t value) {
    char buf[19];
    buf[0] = '0';
    buf[1] = 'x';
    buf[18] = '\0';
    for (int i = 0; i < 16; i++) {
        uint8_t nibble = (value >> ((15 - i) * 4)) & 0xF;
        buf[2 + i] = nibble < 10 ? ('0' + nibble) : ('a' + nibble - 10);
    }
    terminal_write(buf);
}

void terminal_write_dec(uint64_t value) {
    char buf[21];
    int i = 20;
    buf[i--] = '\0';
    if (value == 0) {
        buf[i--] = '0';
    } else {
        while (value > 0 && i >= 0) {
            buf[i--] = '0' + (value % 10);
            value /= 10;
        }
    }
    terminal_write(&buf[i + 1]);
}

void terminal_put_at(uint8_t x, uint8_t y, char c, uint8_t color) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT)
        VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(c, color);
}
