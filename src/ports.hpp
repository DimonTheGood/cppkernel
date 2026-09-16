#pragma once
#include <cstdint>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);