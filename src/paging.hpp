#pragma once
#include <cstdint>

struct PageTableEntry{
    uint32_t present : 1;
    uint32_t writable : 1;
    uint32_t user : 1;
    uint32_t reserved : 9;
    uint32_t frame_addr : 20;
} __attribute__((packed));

void paging_init();