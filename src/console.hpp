#pragma once
#include <cstdint>

constexpr uint8_t VGA_COLOR_DEFAULT = 0x07;
constexpr uint8_t VGA_COLOR_WHITE = 0x0F;
constexpr uint8_t VGA_COLOR_GREEN = 0x0A;
constexpr uint8_t VGA_COLOR_RED = 0x0C;

void console_init();
void console_clear();
void console_set_color(uint8_t color);
void console_putc(char c);
void console_write(const char* message);
void console_write_hex(uint32_t value);
void console_write_dec(uint32_t value);