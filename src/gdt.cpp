#include "gdt.hpp"

static GDTEntry gdt[3];

void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity){
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;
    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].access = access;
    gdt[index].granularity = (granularity << 4) | ((limit >> 16) & 0x0F);
}

void gdt_init(){
    set_gdt_entry(0, 0, 0, 0, 0);
    set_gdt_entry(1, 0, 0xFFFFF, 0x9A, 0xC);
    set_gdt_entry(2, 0, 0xFFFFF, 0x92, 0xC);
    
    GDTPointer gdt_ptr;
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = reinterpret_cast<uint32_t>(gdt);
    asm volatile("lgdt %0" : : "m"(gdt_ptr));
    asm volatile(
        "ljmp $0x08, $reload_cs\n"
        "reload_cs:\n"
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
    );
}