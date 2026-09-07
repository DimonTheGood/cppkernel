#pragma once

#include <cstdint>

struct Multiboot2InfoHeader{
    uint32_t total_size;
    uint32_t reserved;
};

struct Multiboot2Tag{
    uint32_t type;
    uint32_t size;
};

struct Multiboot2MemoryMapTag{
    Multiboot2Tag tag;
    uint32_t entry_size;
    uint32_t entry_version;
};

struct Multiboot2MemoryMapEntry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
};

constexpr uint32_t MULTIBOOT2_TAG_TYPE_END = 0;
constexpr uint32_t MULTIBOOT2_TAG_TYPE_MMAP = 6;
constexpr uint32_t MULTIBOOT2_MEMORY_AVAILABLE = 1;