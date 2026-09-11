#include "idt.hpp"
#include "serial.hpp"
static IDTEntry entries[256];

extern "C" void isr0_handler(){
    serial_write("EXCEPTION: division by zero!\n");
    for(;;){ asm volatile("cli; hlt"); }
}

extern "C" void isr0();

void idt_init(){
    set_idt_entry(0, reinterpret_cast<uint32_t>(isr0), 0x08, 0x8E);
    IDTPointer idt_ptr;
    idt_ptr.limit = sizeof(entries) - 1;
    idt_ptr.base = reinterpret_cast<uint32_t>(entries);
    asm volatile("lidt %0" : : "m"(idt_ptr));
}

void set_idt_entry(int index, uint32_t handler_addr, uint16_t selector, uint8_t type_attr){
    entries[index].offset_low = handler_addr & 0xFFFF;
    entries[index].offset_high = (handler_addr >> 16) & 0xFFFF;
    entries[index].zero = 0;
    entries[index].selector = selector;
    entries[index].type_attr = type_attr;
}