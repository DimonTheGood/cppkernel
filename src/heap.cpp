#include <cstddef>
#include "heap.hpp"
#include "paging.hpp"
#include "serial.hpp"
#include "sync.hpp"

struct BlockHeader{
    uint32_t magic;
    uint32_t size;
    bool free;
    BlockHeader* next;
};

static constexpr uint32_t BLOCK_MAGIC = 0xC0FFEE00;
static constexpr uint32_t MIN_SPLIT_SIZE = 8;

static PhysicalFrameAllocator* pfa_ptr;
static uint32_t heap_current;
static uint32_t heap_end;
static BlockHeader* first_block;
static BlockHeader* last_block;

static void heap_panic(const char* message){
    serial_write("PANIC: ");
    serial_write(message);
    serial_write("\n");
    for(;;){ asm volatile("cli; hlt"); }
}

static bool grow_heap(uint32_t new_current){
    while(new_current > heap_end){
        uint32_t new_frame = pfa_ptr->alloc_frame();
        if(new_frame == 0) return false;
        if(!map_page(heap_end, new_frame)){
            pfa_ptr->free_frame(new_frame);
            return false;
        }
        heap_end += FRAME_SIZE;
    }
    return true;
}

static void split_block(BlockHeader* block, uint32_t size){
    if(block->size < size + sizeof(BlockHeader) + MIN_SPLIT_SIZE) return;
    auto* rest = reinterpret_cast<BlockHeader*>(reinterpret_cast<uint8_t*>(block + 1) + size);
    rest->magic = BLOCK_MAGIC;
    rest->size = block->size - size - sizeof(BlockHeader);
    rest->free = true;
    rest->next = block->next;
    block->size = size;
    block->next = rest;
    if(last_block == block) last_block = rest;
}

static void merge_free_blocks(){
    BlockHeader* block = first_block;
    while(block != nullptr && block->next != nullptr){
        BlockHeader* next = block->next;
        if(block->free && next->free){
            block->size += sizeof(BlockHeader) + next->size;
            block->next = next->next;
            next->magic = 0;
            if(last_block == next) last_block = block;
        } else {
            block = next;
        }
    }
}

static void* kmalloc_locked(uint32_t size){
    if(size == 0 || size > KERNEL_HEAP_END - KERNEL_HEAP_START) return nullptr;
    size = (size + 7) & ~7u;

    for(BlockHeader* block = first_block; block != nullptr; block = block->next){
        if(block->free && block->size >= size){
            split_block(block, size);
            block->free = false;
            return block + 1;
        }
    }

    if(last_block != nullptr && last_block->free){
        uint32_t extra = size - last_block->size;
        if(extra > KERNEL_HEAP_END - heap_current) return nullptr;
        if(!grow_heap(heap_current + extra)) return nullptr;
        heap_current += extra;
        last_block->size = size;
        last_block->free = false;
        return last_block + 1;
    }

    uint32_t needed = sizeof(BlockHeader) + size;
    if(needed > KERNEL_HEAP_END - heap_current) return nullptr;
    if(!grow_heap(heap_current + needed)) return nullptr;
    auto* block = reinterpret_cast<BlockHeader*>(heap_current);
    block->magic = BLOCK_MAGIC;
    block->size = size;
    block->free = false;
    block->next = nullptr;
    if(last_block != nullptr) last_block->next = block;
    else first_block = block;
    last_block = block;
    heap_current += needed;
    return block + 1;
}

void heap_init(PhysicalFrameAllocator* allocator){
    pfa_ptr = allocator;
    heap_current = KERNEL_HEAP_START;
    heap_end = KERNEL_HEAP_START;
    first_block = nullptr;
    last_block = nullptr;
    serial_write("heap init\n");
}

void* kmalloc(uint32_t size){
    uint32_t flags = interrupts_save();
    void* result = kmalloc_locked(size);
    interrupts_restore(flags);
    return result;
}

void kfree(void* ptr){
    if(ptr == nullptr) return;
    uint32_t flags = interrupts_save();
    BlockHeader* block = reinterpret_cast<BlockHeader*>(ptr) - 1;
    if(block->magic != BLOCK_MAGIC) heap_panic("kfree: pointer is not from kmalloc");
    if(block->free) heap_panic("kfree: double free");
    block->free = true;
    merge_free_blocks();
    interrupts_restore(flags);
}

HeapStats heap_stats(){
    uint32_t flags = interrupts_save();
    HeapStats stats{};
    stats.mapped_bytes = heap_end - KERNEL_HEAP_START;
    for(BlockHeader* block = first_block; block != nullptr; block = block->next){
        ++stats.blocks;
        if(block->free) stats.free_bytes += block->size;
        else stats.used_bytes += block->size;
    }
    interrupts_restore(flags);
    return stats;
}

void* operator new(size_t size){
    void* ptr = kmalloc(size);
    if(ptr == nullptr) heap_panic("operator new: out of memory");
    return ptr;
}

void* operator new[](size_t size){
    void* ptr = kmalloc(size);
    if(ptr == nullptr) heap_panic("operator new[]: out of memory");
    return ptr;
}

void operator delete(void* ptr) noexcept{
    kfree(ptr);
}

void operator delete[](void* ptr) noexcept{
    kfree(ptr);
}

void operator delete(void* ptr, size_t) noexcept{
    kfree(ptr);
}

void operator delete[](void* ptr, size_t) noexcept{
    kfree(ptr);
}