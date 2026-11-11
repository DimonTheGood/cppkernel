#include "console.hpp"
#include "ports.hpp"
#include "sync.hpp"

static constexpr uint32_t VGA_WIDTH = 80;
static constexpr uint32_t VGA_HEIGHT = 25;
static constexpr uint32_t TAB_WIDTH = 4;

static volatile uint16_t* const vga = reinterpret_cast<volatile uint16_t*>(0xB8000);
static uint32_t cursor_row = 0;
static uint32_t cursor_col = 0;
static uint8_t color = VGA_COLOR_DEFAULT;

static uint16_t vga_entry(char c){
    return (static_cast<uint16_t>(color) << 8) | static_cast<uint8_t>(c);
}

static void update_cursor(){
    uint16_t position = cursor_row * VGA_WIDTH + cursor_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, position & 0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (position >> 8) & 0xFF);
}

static void scroll(){
    for(uint32_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; ++i){
        vga[i] = vga[i + VGA_WIDTH];
    }
    for(uint32_t col = 0; col < VGA_WIDTH; ++col){
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = vga_entry(' ');
    }
    cursor_row = VGA_HEIGHT - 1;
}

static void new_line(){
    cursor_col = 0;
    ++cursor_row;
    if(cursor_row >= VGA_HEIGHT){
        scroll();
    }
}

static void put_char(char c){
    if(c == '\n'){
        new_line();
    } else if(c == '\r'){
        cursor_col = 0;
    } else if(c == '\b'){
        if(cursor_col > 0){
            --cursor_col;
        } else if(cursor_row > 0){
            --cursor_row;
            cursor_col = VGA_WIDTH - 1;
        }
        vga[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(' ');
    } else if(c == '\t'){
        do{
            put_char(' ');
        } while(cursor_col % TAB_WIDTH != 0);
    } else {
        vga[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(c);
        ++cursor_col;
        if(cursor_col >= VGA_WIDTH){
            new_line();
        }
    }
}

void console_init(){
    color = VGA_COLOR_DEFAULT;
    console_clear();
}

void console_clear(){
    uint32_t flags = interrupts_save();
    for(uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i){
        vga[i] = vga_entry(' ');
    }
    cursor_row = 0;
    cursor_col = 0;
    update_cursor();
    interrupts_restore(flags);
}

void console_set_color(uint8_t new_color){
    color = new_color;
}

void console_putc(char c){
    uint32_t flags = interrupts_save();
    put_char(c);
    update_cursor();
    interrupts_restore(flags);
}

void console_write(const char* message){
    uint32_t flags = interrupts_save();
    while(*message != '\0'){
        put_char(*message);
        ++message;
    }
    update_cursor();
    interrupts_restore(flags);
}

void console_write_hex(uint32_t value){
    const constexpr char digits[] = "0123456789ABCDEF";
    char text[11];
    text[0] = '0';
    text[1] = 'x';
    for(int i = 0; i < 8; ++i){
        text[2 + i] = digits[(value >> (28 - i * 4)) & 0x0F];
    }
    text[10] = '\0';
    console_write(text);
}

void console_write_dec(uint32_t value){
    char text[11];
    int pos = 10;
    text[pos] = '\0';
    do{
        text[--pos] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while(value != 0);
    console_write(text + pos);
}