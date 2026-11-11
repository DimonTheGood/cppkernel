#include "scheduler.hpp"
#include "heap.hpp"
#include "libc.hpp"
#include "pit.hpp"
#include "sync.hpp"

static constexpr uint32_t THREAD_STACK_SIZE = 16384;
static constexpr uint32_t TIME_SLICE_TICKS = 5;

static Thread* threads = nullptr;
static Thread* current = nullptr;
static Thread* idle_thread = nullptr;
static uint32_t next_id = 0;
static uint32_t slice_ticks = 0;

static void idle_loop(){
    for(;;){
        asm volatile("hlt");
    }
}

static void thread_start(){
    current->entry();
    thread_exit();
}

static void append_thread(Thread* thread){
    if(threads == nullptr){
        threads = thread;
        return;
    }
    Thread* tail = threads;
    while(tail->next != nullptr){
        tail = tail->next;
    }
    tail->next = thread;
}

static void reap_dead_threads(){
    Thread* prev = nullptr;
    Thread* thread = threads;
    while(thread != nullptr){
        Thread* next = thread->next;
        if(thread->state == ThreadState::DEAD && thread != current){
            if(prev != nullptr) prev->next = next;
            else threads = next;
            kfree(thread->stack);
            delete thread;
        } else {
            prev = thread;
        }
        thread = next;
    }
}

static Thread* pick_next_thread(){
    Thread* thread = current;
    do{
        thread = thread->next != nullptr ? thread->next : threads;
        if(thread != idle_thread && thread->state == ThreadState::READY){
            return thread;
        }
    } while(thread != current);
    return idle_thread;
}

void scheduler_init(){
    uint32_t flags = interrupts_save();
    Thread* main_thread = new Thread{};
    main_thread->id = next_id++;
    main_thread->name = "main";
    main_thread->state = ThreadState::RUNNING;
    append_thread(main_thread);
    current = main_thread;
    interrupts_restore(flags);
    idle_thread = thread_create("idle", idle_loop);
}

Thread* thread_create(const char* name, void (*entry)()){
    Thread* thread = new Thread{};
    thread->stack = static_cast<uint8_t*>(kmalloc(THREAD_STACK_SIZE));
    if(thread->stack == nullptr){
        delete thread;
        return nullptr;
    }

    uint32_t top = (reinterpret_cast<uint32_t>(thread->stack) + THREAD_STACK_SIZE) & ~15u;
    top -= 4;
    *reinterpret_cast<uint32_t*>(top) = 0;
    auto* frame = reinterpret_cast<Registers*>(top - sizeof(Registers));
    memset(frame, 0, sizeof(Registers));
    frame->eip = reinterpret_cast<uint32_t>(thread_start);
    frame->cs = 0x08;
    frame->eflags = 0x202;

    thread->name = name;
    thread->entry = entry;
    thread->esp = reinterpret_cast<uint32_t>(frame);
    thread->state = ThreadState::READY;

    uint32_t flags = interrupts_save();
    thread->id = next_id++;
    append_thread(thread);
    interrupts_restore(flags);
    return thread;
}

void thread_yield(){
    if(current == nullptr){
        asm volatile("hlt");
        return;
    }
    asm volatile("int %0" : : "i"(YIELD_VECTOR) : "memory");
}

void thread_sleep(uint32_t ms){
    uint32_t ticks = (ms * PIT_FREQUENCY + 999) / 1000;
    if(ticks == 0) ticks = 1;
    uint32_t wake_tick = pit_ticks() + ticks;
    if(current == nullptr){
        while(static_cast<int32_t>(pit_ticks() - wake_tick) < 0){
            asm volatile("hlt");
        }
        return;
    }
    uint32_t flags = interrupts_save();
    current->wake_tick = wake_tick;
    current->state = ThreadState::SLEEPING;
    interrupts_restore(flags);
    thread_yield();
}

void thread_block(){
    uint32_t flags = interrupts_save();
    current->state = ThreadState::BLOCKED;
    asm volatile("int %0" : : "i"(YIELD_VECTOR) : "memory");
    interrupts_restore(flags);
}

void thread_unblock(Thread* thread){
    uint32_t flags = interrupts_save();
    if(thread->state == ThreadState::BLOCKED){
        thread->state = ThreadState::READY;
    }
    interrupts_restore(flags);
}

void thread_exit(){
    asm volatile("cli");
    current->state = ThreadState::DEAD;
    for(;;){
        asm volatile("int %0" : : "i"(YIELD_VECTOR) : "memory");
    }
}

Thread* thread_current(){
    return current;
}

Thread* thread_list(){
    return threads;
}

uint32_t scheduler_switch(Registers* r){
    if(current == nullptr) return reinterpret_cast<uint32_t>(r);
    current->esp = reinterpret_cast<uint32_t>(r);
    if(current->state == ThreadState::RUNNING){
        current->state = ThreadState::READY;
    }
    reap_dead_threads();
    Thread* next = pick_next_thread();
    next->state = ThreadState::RUNNING;
    current = next;
    slice_ticks = 0;
    return next->esp;
}

uint32_t scheduler_on_tick(Registers* r){
    if(current == nullptr) return reinterpret_cast<uint32_t>(r);
    uint32_t now = pit_ticks();
    for(Thread* thread = threads; thread != nullptr; thread = thread->next){
        if(thread->state == ThreadState::SLEEPING && static_cast<int32_t>(now - thread->wake_tick) >= 0){
            thread->state = ThreadState::READY;
        }
    }
    ++slice_ticks;
    if(current != idle_thread && slice_ticks < TIME_SLICE_TICKS){
        return reinterpret_cast<uint32_t>(r);
    }
    return scheduler_switch(r);
}