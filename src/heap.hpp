#pragma once
#include <cstdint>
#include "frame_allocator.hpp"

void heap_init(PhysicalFrameAllocator* allocator);
void* kmalloc(uint32_t size);
void kfree(void* ptr);