#include "pic.hpp"
#include "ports.hpp"

void pic_remap(){
    // ICW1 — начать инициализацию, дальше будет ICW4
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    // ICW2 — смещение векторов (master → 32, slave → 40)
    outb(0x21, 0x20); // master offset
    outb(0xA1, 0x28); // slave offset
    // ICW3 — каскадирование (slave подключён к IRQ2 master'а)
    outb(0x21, 0x04); // master: slave на линии IRQ2
    outb(0xA1, 0x02); // slave: мой номер каскада — 2
    // ICW4 — режим работы (8086/88 mode)
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    // маски — разрешить все прерывания
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}