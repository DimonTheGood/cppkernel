#include <cstdint>
#include "serial.hpp"
#include "multiboot2.hpp"
#include "frame_allocator.hpp"
#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6

struct multiboot_header {
    uint32_t magic;
    uint32_t architecture;
    uint32_t header_length;
    uint32_t checksum;
};

struct multiboot_header_tag_end {
    uint16_t type;
    uint16_t flags;
    uint32_t size;
};

__attribute__((used, section(".multiboot"), aligned(8)))
static const struct multiboot_header mb_header = {
    .magic = MULTIBOOT2_HEADER_MAGIC,
    .architecture = 0, // 0 = i386 
    .header_length = sizeof(struct multiboot_header) + sizeof(struct multiboot_header_tag_end),
    .checksum = 0u - (MULTIBOOT2_HEADER_MAGIC + 0u + (uint32_t)sizeof(struct multiboot_header) + (uint32_t)sizeof(struct multiboot_header_tag_end))
};

__attribute__((used, section(".multiboot"), aligned(8)))
static const struct multiboot_header_tag_end mb_end = {
    .type = 0, .flags = 0, .size = 8
};

extern "C" uint8_t _kernel_start;
extern "C" uint8_t _kernel_end;

extern "C" void kmain(uint32_t magic, uint32_t mbi_addr){
    serial_init();
    constexpr uint32_t MULTIBOOT2_BOOTLOADER_MAGIC = 0x36D76289;
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC){
        serial_write("serial: Invalid magic number\n");
        for(;;){
            asm volatile("cli; hlt");
        }
    }
    const auto* mbi = reinterpret_cast<const Multiboot2InfoHeader*>(mbi_addr);
    serial_write("magic passed!\n");
    serial_write("serial: kmain reached\n");
    serial_write("serial: mbi_addr = ");
    serial_write_hex(mbi_addr);
    serial_write("\n");
    serial_write("serial: mbi->total_size = ");
    serial_write_hex(mbi->total_size);
    serial_write("\n");
    const uint8_t* mbi_start = reinterpret_cast<const uint8_t*>(mbi);

    const uint8_t* mbi_end = mbi_start + mbi->total_size;

    const uint8_t* cursor = mbi_start + sizeof(Multiboot2InfoHeader);
    while(cursor < mbi_end){
        const auto* tag = reinterpret_cast<const Multiboot2Tag*>(cursor);
        serial_write("serial: tag->type = ");
        serial_write_hex(tag->type);
        serial_write(", size: ");
        serial_write_hex(tag->size);
        serial_write("\n");
        if(tag->type == MULTIBOOT2_TAG_TYPE_MMAP){
            const auto* mmap_tag = reinterpret_cast<const Multiboot2MemoryMapTag*>(tag);
            serial_write("memory map entry size: ");
            serial_write_hex(mmap_tag->entry_size);
            serial_write("\n");
            if(mmap_tag->entry_size < sizeof(Multiboot2MemoryMapEntry)){
                serial_write("PANIC: mmap_tag size to small!!");
                break;
            }
            const uint8_t* entry_cursor = reinterpret_cast<const uint8_t*>(mmap_tag) + sizeof(Multiboot2MemoryMapTag);
            const uint8_t* entries_end = reinterpret_cast<const uint8_t*>(mmap_tag) + mmap_tag->tag.size;
            uint32_t entry_index = 0;
            while(entry_cursor + mmap_tag->entry_size <= entries_end){
                const auto* entry = reinterpret_cast<const Multiboot2MemoryMapEntry*>(entry_cursor);
                serial_write("memory region ");
                serial_write_hex(entry_index);
                serial_write(": addr=");
                serial_write_hex64(entry->addr);
                serial_write(", len=");
                serial_write_hex64(entry->len);
                serial_write(", type=");
                serial_write_hex(entry->type);
                serial_write("\n");
                entry_cursor += mmap_tag->entry_size;
                ++entry_index;
            }
            static PhysicalFrameAllocator pfa;
            pfa.init(mmap_tag, reinterpret_cast<uint32_t>(&_kernel_start), reinterpret_cast<uint32_t>(&_kernel_end));
            serial_write("frame allocator initialized\n");
            uint32_t f1 = pfa.alloc_frame();
            serial_write("allocated frame: "); serial_write_hex(f1); serial_write("\n");
        }
        if(tag->type == MULTIBOOT2_TAG_TYPE_END){
            break;
        }
        if(tag->size < sizeof(Multiboot2Tag)){
            serial_write("PANIC: Invalid tag size\n");
            break;
        }
        uint32_t aligned_size = (tag->size + 7u) & ~7u;
        if (cursor + aligned_size > mbi_end) {
            serial_write("PANIC: tag exceeds Multiboot2 block\n");
            break;
        }
        cursor += aligned_size;
    }
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000; // vga text 80x25
 
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = 0x0720;
    }

    const char* msg = "kmain reached";
    int len = 13;
    int col = (80 - len) / 2;
    int row = 12;
    
    for (int i = 0; i < len; i++) {
        vga[(row * 80) + col + i] = 0x0F00 | msg[i]; 
    }

    for(;;);
}