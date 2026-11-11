#include "heap.hpp"
#include "paging.hpp"
#include "serial.hpp"

static PhysicalFrameAllocator* pfa_ptr;
static uint32_t heap_current;
static uint32_t heap_end;

void heap_init(PhysicalFrameAllocator* allocator){
    pfa_ptr = allocator;
    heap_current = KERNEL_HEAP_START;
    heap_end = KERNEL_HEAP_START;
    serial_write("heap init\n");
}

void* kmalloc(uint32_t size){
    if(size == 0 || size > KERNEL_HEAP_END - heap_current) return nullptr;
    size = (size + 3) & ~3u;
    while(heap_current + size > heap_end){
        uint32_t new_frame = pfa_ptr->alloc_frame();
        if(new_frame == 0) return nullptr;
        if(!map_page(heap_end, new_frame)){
            pfa_ptr->free_frame(new_frame);
            return nullptr;
        }
        heap_end += FRAME_SIZE;
    }
    uint32_t addr = heap_current;
    heap_current += size;
    return reinterpret_cast<void*>(addr);
}

void kfree(void* ptr){
    // пока ничего не делаем — bump allocator не умеет освобождать
    (void)ptr; // подавляем warning о неиспользуемом параметре
}