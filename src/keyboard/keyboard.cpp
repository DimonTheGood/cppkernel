#include <cstdint>
#include "keyboard.hpp"
#include "ports.hpp"
#include "scheduler.hpp"
#include "sync.hpp"

static constexpr uint32_t BUFFER_SIZE = 256;
static constexpr uint8_t SCANCODE_LEFT_SHIFT = 0x2A;
static constexpr uint8_t SCANCODE_RIGHT_SHIFT = 0x36;
static constexpr uint8_t SCANCODE_CAPS_LOCK = 0x3A;
static constexpr uint8_t SCANCODE_EXTENDED = 0xE0;
static constexpr uint8_t SCANCODE_RELEASE = 0x80;

static const char scancode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, // left ctrl
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, // left shift
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, // right shift
    '*',
    0, // left alt
    ' ', // space
    // остальное пока не используем — 0
};

static const char scancode_to_ascii_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,
    '*',
    0,
    ' ',
};

static volatile char buffer[BUFFER_SIZE];
static volatile uint32_t buffer_head = 0;
static volatile uint32_t buffer_tail = 0;
static bool shift_pressed = false;
static bool caps_lock = false;
static bool extended_code = false;
static Thread* waiting_thread = nullptr;

static bool is_letter(char c){
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void buffer_push(char c){
    uint32_t next = (buffer_head + 1) % BUFFER_SIZE;
    if(next == buffer_tail) return;
    buffer[buffer_head] = c;
    buffer_head = next;
}

void keyboard_handle_irq(){
    uint8_t scan_code = inb(0x60);
    if(scan_code == SCANCODE_EXTENDED){
        extended_code = true;
        return;
    }
    if(extended_code){
        extended_code = false;
        return;
    }

    bool released = scan_code & SCANCODE_RELEASE;
    uint8_t key = scan_code & ~SCANCODE_RELEASE;
    if(key == SCANCODE_LEFT_SHIFT || key == SCANCODE_RIGHT_SHIFT){
        shift_pressed = !released;
        return;
    }
    if(released) return;
    if(key == SCANCODE_CAPS_LOCK){
        caps_lock = !caps_lock;
        return;
    }

    char ascii = shift_pressed ? scancode_to_ascii_shift[key] : scancode_to_ascii[key];
    if(caps_lock && is_letter(ascii)){
        ascii = shift_pressed ? scancode_to_ascii[key] : scancode_to_ascii_shift[key];
    }
    if(ascii != 0){
        buffer_push(ascii);
        if(waiting_thread != nullptr){
            thread_unblock(waiting_thread);
            waiting_thread = nullptr;
        }
    }
}

bool keyboard_has_char(){
    return buffer_head != buffer_tail;
}

char keyboard_read_char(){
    for(;;){
        uint32_t flags = interrupts_save();
        if(buffer_head != buffer_tail){
            char c = buffer[buffer_tail];
            buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
            interrupts_restore(flags);
            return c;
        }
        if(thread_current() == nullptr){
            asm volatile("sti; hlt" : : : "memory");
        } else {
            waiting_thread = thread_current();
            thread_block();
        }
        interrupts_restore(flags);
    }
}