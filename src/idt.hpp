#pragma once
#include <cstdint>

struct IDTEntry{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IDTPointer{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void set_idt_entry(int index, uint32_t handler_addr, uint16_t selector, uint8_t type_attr);
void idt_init();