#pragma once

#include <cstdint>
void serial_init();
void serial_write(const char* message);
void serial_write_hex(uint32_t value);
void serial_write_hex64(uint64_t value);