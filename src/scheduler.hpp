#pragma once
#include <cstdint>
#include "idt.hpp"

constexpr uint32_t YIELD_VECTOR = 0x81;

enum class ThreadState{
    READY,
    RUNNING,
    SLEEPING,
    BLOCKED,
    DEAD
};

struct Thread{
    uint32_t id;
    const char* name;
    uint32_t esp;
    uint8_t* stack;
    void (*entry)();
    ThreadState state;
    uint32_t wake_tick;
    Thread* next;
};

void scheduler_init();
Thread* thread_create(const char* name, void (*entry)());
void thread_yield();
void thread_sleep(uint32_t ms);
void thread_block();
void thread_unblock(Thread* thread);
[[noreturn]] void thread_exit();
Thread* thread_current();
Thread* thread_list();

uint32_t scheduler_on_tick(Registers* r);
uint32_t scheduler_switch(Registers* r);