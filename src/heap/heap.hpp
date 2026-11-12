#pragma once
#include <cstdint>
#include "frame_allocator.hpp"

struct HeapStats{
    uint32_t mapped_bytes;
    uint32_t used_bytes;
    uint32_t free_bytes;
    uint32_t blocks;
};

void heap_init(PhysicalFrameAllocator* allocator);
void* kmalloc(uint32_t size);
void kfree(void* ptr);
HeapStats heap_stats();