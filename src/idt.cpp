#include "idt.hpp"
#include "serial.hpp"
#include "ports.hpp"
static IDTEntry entries[256];

extern "C" void isr_handler(int vector_number){
    if(vector_number == 32){
        static uint32_t tick_count = 0;
        ++tick_count;
        serial_write("tick: ");
        serial_write_hex(tick_count);
        serial_write("\n");
        outb(0x20, 0x20);  
        return; 
    }
    serial_write("EXCEPTION: vector ");
    serial_write_hex(vector_number);
    serial_write("\n");
    for(;;){ asm volatile("cli; hlt"); }
}

extern "C" void isr0();  extern "C" void isr1();  extern "C" void isr2();  extern "C" void isr3();
extern "C" void isr4();  extern "C" void isr5();  extern "C" void isr6();  extern "C" void isr7();
extern "C" void isr8();  extern "C" void isr9();  extern "C" void isr10(); extern "C" void isr11();
extern "C" void isr12(); extern "C" void isr13(); extern "C" void isr14(); extern "C" void isr15();
extern "C" void isr16(); extern "C" void isr17(); extern "C" void isr18(); extern "C" void isr19();
extern "C" void isr20(); extern "C" void isr21(); extern "C" void isr22(); extern "C" void isr23();
extern "C" void isr24(); extern "C" void isr25(); extern "C" void isr26(); extern "C" void isr27();
extern "C" void isr28(); extern "C" void isr29(); extern "C" void isr30(); extern "C" void isr31();
extern "C" void isr32();
extern "C" void isr_default();
static uint32_t isr_addresses[32] = {
    reinterpret_cast<uint32_t>(isr0),  reinterpret_cast<uint32_t>(isr1),
    reinterpret_cast<uint32_t>(isr2),  reinterpret_cast<uint32_t>(isr3),
    reinterpret_cast<uint32_t>(isr4),  reinterpret_cast<uint32_t>(isr5),
    reinterpret_cast<uint32_t>(isr6),  reinterpret_cast<uint32_t>(isr7),
    reinterpret_cast<uint32_t>(isr8),  reinterpret_cast<uint32_t>(isr9),
    reinterpret_cast<uint32_t>(isr10), reinterpret_cast<uint32_t>(isr11),
    reinterpret_cast<uint32_t>(isr12), reinterpret_cast<uint32_t>(isr13),
    reinterpret_cast<uint32_t>(isr14), reinterpret_cast<uint32_t>(isr15),
    reinterpret_cast<uint32_t>(isr16), reinterpret_cast<uint32_t>(isr17),
    reinterpret_cast<uint32_t>(isr18), reinterpret_cast<uint32_t>(isr19),
    reinterpret_cast<uint32_t>(isr20), reinterpret_cast<uint32_t>(isr21),
    reinterpret_cast<uint32_t>(isr22), reinterpret_cast<uint32_t>(isr23),
    reinterpret_cast<uint32_t>(isr24), reinterpret_cast<uint32_t>(isr25),
    reinterpret_cast<uint32_t>(isr26), reinterpret_cast<uint32_t>(isr27),
    reinterpret_cast<uint32_t>(isr28), reinterpret_cast<uint32_t>(isr29),
    reinterpret_cast<uint32_t>(isr30), reinterpret_cast<uint32_t>(isr31)
};

void set_idt_entry(int index, uint32_t handler_addr, uint16_t selector, uint8_t type_attr){
    entries[index].offset_low = handler_addr & 0xFFFF;
    entries[index].offset_high = (handler_addr >> 16) & 0xFFFF;
    entries[index].zero = 0;
    entries[index].selector = selector;
    entries[index].type_attr = type_attr;
}

void idt_init(){
    for(int i = 0; i < 32; ++i){
        set_idt_entry(i, isr_addresses[i], 0x08, 0x8E);
    }
    for(int i = 32; i < 256; ++i){
        set_idt_entry(i, reinterpret_cast<uint32_t>(isr_default), 0x08, 0x8E);
    }
    set_idt_entry(32, reinterpret_cast<uint32_t>(isr32), 0x08, 0x8E);
    IDTPointer idt_ptr;
    idt_ptr.limit = sizeof(entries) - 1;
    idt_ptr.base = reinterpret_cast<uint32_t>(entries);
    asm volatile("lidt %0" : : "m"(idt_ptr));
}