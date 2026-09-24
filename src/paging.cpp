#include "paging.hpp"
#include "serial.hpp"

alignas(4096) static PageTableEntry page_directory[1024];
alignas(4096) static PageTableEntry first_page_table[1024];

void paging_init(){
    for(int i = 0; i < 1024; ++i){
        first_page_table[i].frame_addr = i;
        first_page_table[i].present = 1;
        first_page_table[i].writable = 1;
    }
    page_directory[0].frame_addr = reinterpret_cast<uint32_t>(first_page_table) >> 12;
    page_directory[0].present = 1;
    page_directory[0].writable = 1;

    asm volatile("mov %0, %%cr3" : : "r"(page_directory));

    uint32_t cr0_value;
    asm volatile("mov %%cr0, %0" : "=r"(cr0_value));
    cr0_value |= (1 << 31);
    asm volatile("mov %0, %%cr0" : : "r"(cr0_value));
    serial_write("paging enabled\n");
}