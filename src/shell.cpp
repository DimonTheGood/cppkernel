#include <cstdint>
#include "shell.hpp"
#include "console.hpp"
#include "heap.hpp"
#include "keyboard.hpp"
#include "libc.hpp"
#include "pit.hpp"
#include "ports.hpp"
#include "scheduler.hpp"
#include "serial.hpp"
#include "sync.hpp"

static constexpr uint32_t LINE_SIZE = 128;
static constexpr uint32_t MAX_ARGS = 8;

struct Command{
    const char* name;
    const char* help;
    void (*handler)(uint32_t argc, char** argv);
};

static PhysicalFrameAllocator* pfa_ptr = nullptr;
static volatile uint32_t spin_count = 0;
static uint8_t text_color = VGA_COLOR_DEFAULT;

static void print(const char* message){
    console_write(message);
    serial_write(message);
}

static void print_char(char c){
    char text[2] = {c, '\0'};
    print(text);
}

static const char* dec_to_string(uint32_t value, char* text){
    int pos = 10;
    text[pos] = '\0';
    do{
        text[--pos] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while(value != 0);
    return text + pos;
}

static void print_dec(uint32_t value){
    char text[11];
    print(dec_to_string(value, text));
}

static void print_padded(const char* text, uint32_t width){
    print(text);
    for(uint32_t i = strlen(text); i < width; ++i){
        print_char(' ');
    }
}

static bool parse_uint(const char* text, uint32_t* value){
    if(*text == '\0') return false;
    uint32_t result = 0;
    while(*text != '\0'){
        if(*text < '0' || *text > '9') return false;
        uint32_t digit = *text - '0';
        if(result > (0xFFFFFFFFu - digit) / 10) return false;
        result = result * 10 + digit;
        ++text;
    }
    *value = result;
    return true;
}

static const char* state_name(ThreadState state){
    switch(state){
        case ThreadState::READY: return "ready";
        case ThreadState::RUNNING: return "running";
        case ThreadState::SLEEPING: return "sleeping";
        case ThreadState::BLOCKED: return "blocked";
        case ThreadState::DEAD: return "dead";
    }
    return "?";
}

static uint32_t read_line(char* line, uint32_t size){
    uint32_t length = 0;
    for(;;){
        char c = keyboard_read_char();
        if(c == '\n'){
            print_char('\n');
            line[length] = '\0';
            return length;
        }
        if(c == '\b'){
            if(length > 0){
                --length;
                print_char('\b');
            }
            continue;
        }
        if(c == '\t' || c == 27) continue;
        if(length + 1 < size){
            line[length++] = c;
            print_char(c);
        }
    }
}

static uint32_t split_args(char* line, char** argv){
    uint32_t argc = 0;
    char* cursor = line;
    while(argc < MAX_ARGS){
        while(*cursor == ' '){
            ++cursor;
        }
        if(*cursor == '\0') break;
        argv[argc++] = cursor;
        while(*cursor != ' ' && *cursor != '\0'){
            ++cursor;
        }
        if(*cursor == '\0') break;
        *cursor = '\0';
        ++cursor;
    }
    return argc;
}

static void run_counter(uint32_t delay_ms){
    Thread* self = thread_current();
    for(uint32_t i = 1; i <= 5; ++i){
        print(self->name);
        print(": step ");
        print_dec(i);
        print("\n");
        thread_sleep(delay_ms);
    }
}

static void counter_a(){
    run_counter(300);
}

static void counter_b(){
    run_counter(500);
}

static void spinner(){
    uint32_t end_tick = pit_ticks() + 3 * PIT_FREQUENCY;
    while(static_cast<int32_t>(pit_ticks() - end_tick) < 0){
        spin_count = spin_count + 1;
    }
    print("spinner: done, ");
    print_dec(spin_count);
    print(" iterations\n");
}

static void cmd_help(uint32_t argc, char** argv);

static void cmd_clear(uint32_t, char**){
    console_clear();
}

static void cmd_echo(uint32_t argc, char** argv){
    for(uint32_t i = 1; i < argc; ++i){
        if(i > 1) print_char(' ');
        print(argv[i]);
    }
    print_char('\n');
}

static void cmd_uptime(uint32_t, char**){
    uint32_t ticks = pit_ticks();
    uint32_t seconds = ticks / PIT_FREQUENCY;
    uint32_t hundredths = (ticks % PIT_FREQUENCY) * 100 / PIT_FREQUENCY;
    print("uptime: ");
    print_dec(seconds);
    print_char('.');
    if(hundredths < 10) print_char('0');
    print_dec(hundredths);
    print(" s (");
    print_dec(ticks);
    print(" ticks)\n");
}

static void cmd_mem(uint32_t, char**){
    uint32_t total = pfa_ptr->total_frames();
    uint32_t free = pfa_ptr->free_frames();
    print("frames: ");
    print_dec(total - free);
    print(" used / ");
    print_dec(total);
    print(" total (");
    print_dec(free * (FRAME_SIZE / 1024));
    print(" KB free)\n");

    HeapStats stats = heap_stats();
    print("heap: ");
    print_dec(stats.used_bytes);
    print(" used, ");
    print_dec(stats.free_bytes);
    print(" free, ");
    print_dec(stats.mapped_bytes);
    print(" mapped bytes, ");
    print_dec(stats.blocks);
    print(" blocks\n");
}

static void cmd_ps(uint32_t, char**){
    print_padded("ID", 5);
    print_padded("STATE", 10);
    print("NAME\n");
    uint32_t flags = interrupts_save();
    for(Thread* thread = thread_list(); thread != nullptr; thread = thread->next){
        char id[11];
        print_padded(dec_to_string(thread->id, id), 5);
        print_padded(state_name(thread->state), 10);
        print(thread->name);
        print_char('\n');
    }
    interrupts_restore(flags);
}

static void cmd_sleep(uint32_t argc, char** argv){
    uint32_t ms;
    if(argc != 2 || !parse_uint(argv[1], &ms)){
        print("usage: sleep <ms>\n");
        return;
    }
    thread_sleep(ms);
}

static void cmd_demo(uint32_t, char**){
    spin_count = 0;
    thread_create("counter-a", counter_a);
    thread_create("counter-b", counter_b);
    thread_create("spinner", spinner);
}

static void cmd_color(uint32_t argc, char** argv){
    if(argc != 2){
        print("usage: color <default|white|green|red>\n");
        return;
    }
    if(strcmp(argv[1], "default") == 0) text_color = VGA_COLOR_DEFAULT;
    else if(strcmp(argv[1], "white") == 0) text_color = VGA_COLOR_WHITE;
    else if(strcmp(argv[1], "green") == 0) text_color = VGA_COLOR_GREEN;
    else if(strcmp(argv[1], "red") == 0) text_color = VGA_COLOR_RED;
    else print("unknown color\n");
    console_set_color(text_color);
}

static void cmd_reboot(uint32_t, char**){
    print("rebooting...\n");
    asm volatile("cli");
    while(inb(0x64) & 0x02){}
    outb(0x64, 0xFE);
    for(;;){
        asm volatile("hlt");
    }
}

static void cmd_halt(uint32_t, char**){
    print("system halted\n");
    for(;;){
        asm volatile("cli; hlt");
    }
}

static const Command commands[] = {
    {"help", "show this list", cmd_help},
    {"clear", "clear the screen", cmd_clear},
    {"echo", "print arguments", cmd_echo},
    {"uptime", "time since boot", cmd_uptime},
    {"mem", "frame allocator and heap stats", cmd_mem},
    {"ps", "list threads", cmd_ps},
    {"sleep", "sleep <ms>", cmd_sleep},
    {"demo", "start counter and spinner threads", cmd_demo},
    {"color", "color <default|white|green|red>", cmd_color},
    {"reboot", "reboot the machine", cmd_reboot},
    {"halt", "stop the cpu", cmd_halt},
};

static constexpr uint32_t COMMAND_COUNT = sizeof(commands) / sizeof(commands[0]);

static void cmd_help(uint32_t, char**){
    for(uint32_t i = 0; i < COMMAND_COUNT; ++i){
        print_padded(commands[i].name, 10);
        print(commands[i].help);
        print_char('\n');
    }
}

static void execute(char* line){
    char* argv[MAX_ARGS];
    uint32_t argc = split_args(line, argv);
    if(argc == 0) return;
    for(uint32_t i = 0; i < COMMAND_COUNT; ++i){
        if(strcmp(argv[0], commands[i].name) == 0){
            commands[i].handler(argc, argv);
            return;
        }
    }
    print("unknown command: ");
    print(argv[0]);
    print(", type help\n");
}

void shell_run(PhysicalFrameAllocator* allocator){
    pfa_ptr = allocator;
    char line[LINE_SIZE];
    print("type help for commands\n");
    for(;;){
        console_set_color(VGA_COLOR_GREEN);
        print("> ");
        console_set_color(text_color);
        read_line(line, LINE_SIZE);
        execute(line);
    }
}
