#pragma once
#include <cstdint>
#include "multiboot2.hpp"


class PhysicalFrameAllocator{
public:
    void init(const Multiboot2MemoryMapTag* mmap_tag, uint32_t kernel_start, uint32_t kernel_end);
    uint32_t alloc_frame();
    void free_frame(uint32_t addr);
private:
    void set_frame(uint32_t addr);
    void clear_frame(uint32_t addr);
    bool test_frame(uint32_t addr);
    void mark_range(uint64_t addr, uint64_t len, bool used);
    uint8_t* bitmap_;
    uint32_t total_frames_;
    uint32_t bitmap_size_bytes_;
};

constexpr uint32_t FRAME_SIZE = 4096;