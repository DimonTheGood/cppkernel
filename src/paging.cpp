#include "paging.hpp"
#include "serial.hpp"
#include "libc.hpp"

alignas(4096) static PageTableEntry page_directory[1024];
static PhysicalFrameAllocator* pfa_ptr;

bool map_page(uint32_t virt, uint32_t phys){
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    PageTableEntry& pde = page_directory[pd_index];
    if(!pde.present){
        uint32_t pt_frame = pfa_ptr->alloc_frame();
        if(pt_frame == 0) return false;
        memset(reinterpret_cast<void*>(pt_frame), 0, FRAME_SIZE);
        pde.frame_addr = pt_frame >> 12;
        pde.present = 1;
        pde.writable = 1;
    }
    auto* page_table = reinterpret_cast<PageTableEntry*>(static_cast<uint32_t>(pde.frame_addr) << 12);
    page_table[pt_index].frame_addr = phys >> 12;
    page_table[pt_index].present = 1;
    page_table[pt_index].writable = 1;
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    return true;
}

void paging_init(PhysicalFrameAllocator* allocator){
    pfa_ptr = allocator;
    uint64_t identity_end = static_cast<uint64_t>(allocator->total_frames()) * FRAME_SIZE;
    if(identity_end > KERNEL_HEAP_START){
        identity_end = KERNEL_HEAP_START;
    }
    for(uint64_t addr = FRAME_SIZE; addr < identity_end; addr += FRAME_SIZE){
        if(!map_page(static_cast<uint32_t>(addr), static_cast<uint32_t>(addr))){
            serial_write("PANIC: out of frames for page tables\n");
            for(;;){ asm volatile("cli; hlt"); }
        }
    }

    asm volatile("mov %0, %%cr3" : : "r"(page_directory) : "memory");

    uint32_t cr0_value;
    asm volatile("mov %%cr0, %0" : "=r"(cr0_value));
    cr0_value |= (1u << 31);
    asm volatile("mov %0, %%cr0" : : "r"(cr0_value) : "memory");
    serial_write("paging enabled\n");
}