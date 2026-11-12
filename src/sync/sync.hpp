#pragma once
#include <cstdint>

inline uint32_t interrupts_save(){
    uint32_t flags;
    asm volatile("pushf\n"
                 "pop %0\n"
                 "cli"
                 : "=r"(flags) : : "memory");
    return flags;
}

inline void interrupts_restore(uint32_t flags){
    asm volatile("push %0\n"
                 "popf"
                 : : "r"(flags) : "memory", "cc");
}