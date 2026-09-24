#include "heap.hpp"
#include "serial.hpp"

static PhysicalFrameAllocator* pfa_ptr;
static uint32_t heap_current;
static uint32_t heap_end;

void heap_init(PhysicalFrameAllocator* allocator){
    pfa_ptr = allocator;
    heap_end = 0;
    heap_current = 0;
    serial_write("heap init\n");
}

void* kmalloc(uint32_t size){
    size = (size + 3) & ~3;
    while(heap_current + size > heap_end){
        uint32_t new_frame = pfa_ptr->alloc_frame();
        if(new_frame == 0) return nullptr;
        if(heap_end == 0) heap_current = new_frame;
        heap_end = heap_current + 4096;
    }
    uint32_t addr = heap_current;
    heap_current += size;
    return reinterpret_cast<void*>(addr);
}

void kfree(void* ptr){
    // пока ничего не делаем — bump allocator не умеет освобождать
    (void)ptr; // подавляем warning о неиспользуемом параметре
}