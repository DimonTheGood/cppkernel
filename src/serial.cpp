#include <cstdint>
#include "serial.hpp"
static constexpr uint16_t COM1 = 0x3F8;

static inline void outb(uint16_t port, uint8_t value){
    asm volatile ("outb %0, %1": : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port){
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void serial_init() {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static bool serial_can_transmit(){
    return (inb(COM1 + 5) & 0x20) != 0;
}

static void serial_write_char(char character){
    while(!serial_can_transmit()){};
    outb(COM1, static_cast<uint8_t>(character));
}

void serial_write(const char* message){
    while(*message != '\0'){
        if(*message == '\n'){
            serial_write_char('\r');
        }
        serial_write_char(*message);
        ++message;
    }
}

void serial_write_hex(uint32_t value){
    const constexpr char digits[] = "0123456789ABCDEF";
    serial_write("0x");
    for(int shift = 28; shift >= 0; shift -= 4){
        uint8_t digit = (value >> shift) & 0x0F;
        serial_write_char(digits[digit]);
    }
}

void serial_write_hex64(uint64_t value) {
    static constexpr char digits[] = "0123456789ABCDEF";
    serial_write("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        uint8_t digit = static_cast<uint8_t>((value >> shift) & 0x0F);
        serial_write_char(digits[digit]);
    }
}