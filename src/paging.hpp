#pragma once
#include <cstdint>
#include "frame_allocator.hpp"

struct PageTableEntry{
    uint32_t present : 1;
    uint32_t writable : 1;
    uint32_t user : 1;
    uint32_t reserved : 9;
    uint32_t frame_addr : 20;
} __attribute__((packed));

constexpr uint32_t KERNEL_HEAP_START = 0xD0000000;
constexpr uint32_t KERNEL_HEAP_END = 0xF0000000;

void paging_init(PhysicalFrameAllocator* allocator);
bool map_page(uint32_t virt, uint32_t phys);