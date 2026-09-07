#include "frame_allocator.hpp"
#include "libc.hpp"

void PhysicalFrameAllocator::set_frame(uint32_t addr){
    uint32_t frame = addr / FRAME_SIZE;
    uint32_t byte_index = frame / 8;
    uint8_t bit_index = frame % 8;
    bitmap_[byte_index] |= (1 << bit_index);
}

void PhysicalFrameAllocator::clear_frame(uint32_t addr){
    uint32_t frame = addr / FRAME_SIZE;
    uint32_t byte_index = frame / 8;
    uint8_t bit_index = frame % 8;
    bitmap_[byte_index] &= ~(1 << bit_index);
}

bool PhysicalFrameAllocator::test_frame(uint32_t addr){
    uint32_t frame = addr / FRAME_SIZE;
    uint32_t byte_index = frame / 8;
    uint8_t bit_index = frame % 8;
    return (bitmap_[byte_index] >> bit_index) & 1;
}

void PhysicalFrameAllocator::mark_range(uint64_t addr, uint64_t len, bool used){
    uint64_t end_addr = addr + len;
    if(addr >= 0x100000000ULL){
        return;
    }
    if(end_addr > 0x100000000ULL){
        end_addr = 0x100000000ULL;
    }
    uint32_t start_frame = static_cast<uint32_t>(addr / FRAME_SIZE);
    uint32_t end_frame = static_cast<uint32_t>(end_addr / FRAME_SIZE);
    for(uint32_t f = start_frame; f < end_frame; ++f){
        uint32_t frame_addr = f * FRAME_SIZE;
        if(used) set_frame(frame_addr);
        else clear_frame(frame_addr);
    }
}

void PhysicalFrameAllocator::init(const Multiboot2MemoryMapTag* mmap_tag, uint32_t kernel_start, uint32_t kernel_end){
    uint64_t max_addr = 0;
    const uint8_t* entry_cursor = reinterpret_cast<const uint8_t*>(mmap_tag) + sizeof(Multiboot2MemoryMapTag);
    const uint8_t* entries_end = reinterpret_cast<const uint8_t*>(mmap_tag) + mmap_tag->tag.size;
    while(entry_cursor + mmap_tag->entry_size <= entries_end){
        const auto* entry = reinterpret_cast<const Multiboot2MemoryMapEntry*>(entry_cursor);
        if(entry->addr + entry->len > max_addr){
            max_addr = entry->addr + entry->len;
        }
        entry_cursor += mmap_tag->entry_size;
    }
    if(max_addr > 0xFFFFFFFF){
        max_addr = 0xFFFFFFFF;
    }
    total_frames_ = static_cast<uint32_t>(max_addr / FRAME_SIZE);
    bitmap_size_bytes_ = (total_frames_ + 7) / 8;

    bitmap_ = reinterpret_cast<uint8_t*>(kernel_end);

    memset(bitmap_, 0xFF, bitmap_size_bytes_);

    entry_cursor = reinterpret_cast<const uint8_t*>(mmap_tag) + sizeof(Multiboot2MemoryMapTag);
    while(entry_cursor + mmap_tag->entry_size <= entries_end){
        const auto* entry = reinterpret_cast<const Multiboot2MemoryMapEntry*>(entry_cursor);
        if(entry->type == MULTIBOOT2_MEMORY_AVAILABLE){
            mark_range(entry->addr, entry->len, false);
        }
        entry_cursor += mmap_tag->entry_size;
    }
    mark_range(0, 0x100000, true);                          
    mark_range(kernel_start, kernel_end - kernel_start, true); 
    mark_range(reinterpret_cast<uint32_t>(bitmap_), bitmap_size_bytes_, true); 
}

uint32_t PhysicalFrameAllocator::alloc_frame(){
    for(uint32_t frame = 0; frame < total_frames_; ++frame){
        uint32_t frame_addr = frame * FRAME_SIZE;
        if(!test_frame(frame_addr)){
            set_frame(frame_addr);
            return frame_addr;
        }
    }
    return 0;
}

void PhysicalFrameAllocator::free_frame(uint32_t addr){
    clear_frame(addr);
}