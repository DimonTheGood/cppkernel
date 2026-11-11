#include "pit.hpp"
#include "ports.hpp"

static volatile uint32_t tick_count = 0;

void pit_init(){
    uint32_t divider = 1193182 / PIT_FREQUENCY;
    outb(0x43, 0x36);
    outb(0x40, divider & 0xFF);
    outb(0x40, (divider >> 8) & 0xFF);
}

void pit_tick(){
    tick_count = tick_count + 1;
}

uint32_t pit_ticks(){
    return tick_count;
}