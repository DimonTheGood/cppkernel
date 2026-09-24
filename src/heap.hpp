#pragma once
#include <cstdint>

void heap_init(uint32_t start_addr, uint32_t size);
void* kmalloc(uint32_t size);
void kfree(void* ptr);