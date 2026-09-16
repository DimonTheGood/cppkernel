#include "pit.hpp"
#include "ports.hpp"

void pit_init(){
    uint32_t divider = 1193182 / 100;
    outb(0x43, 0x36);
    outb(0x40, divider & 0xFF);    
    outb(0x40, (divider >> 8) & 0xFF); 
}