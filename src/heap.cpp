#include "heap.hpp"
#include "serial.hpp"

static uint32_t heap_start;
static uint32_t heap_current;
static uint32_t heap_end;

void heap_init(uint32_t start_addr, uint32_t size){
    heap_start = start_addr;
    heap_end = start_addr + size;
    heap_current = start_addr;
}

void* kmalloc(uint32_t size){
    size = (size + 3) & ~3;
    if(heap_current + size > heap_end){
        return nullptr;
    }
    uint32_t tmp_addr = heap_current;
    heap_current = heap_current + size;
    return reinterpret_cast<void*>(tmp_addr);
}

void kfree(void* ptr){
    // пока ничего не делаем — bump allocator не умеет освобождать
    (void)ptr; // подавляем warning о неиспользуемом параметре
}