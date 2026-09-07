#pragma once
#include <cstdint>

void* memset(void* dest, int value, uint32_t count);
void* memcpy(void* dest, const void* src, uint32_t count);
void* memmove(void* dest, const void* src, uint32_t count);
int memcmp(const void* a, const void* b, uint32_t count);
uint32_t strlen(const char* str);