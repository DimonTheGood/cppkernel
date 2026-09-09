CXX := x86_64-linux-gnu-g++
LD := x86_64-linux-gnu-ld
QEMU := qemu-system-x86_64
GRUB_MKRESCUE := grub-mkrescue

CXXFLAGS := -m32 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -fno-pic -fno-pie -fno-stack-protector -Wall -Wextra -O2
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib --no-dynamic-linker

CPP_SRC := src/kernel.cpp src/serial.cpp src/frame_allocator.cpp src/libc.cpp src/gdt.cpp
ASM_SRC := src/boot.S
OBJ := $(CPP_SRC:.cpp=.o) $(ASM_SRC:.S=.o)
ISO := kernel.iso

.PHONY: all run clean

all: $(ISO)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.S
	$(CXX) $(CXXFLAGS) -c $< -o $@

kernel.bin: $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)
	grub-file --is-x86-multiboot2 $@

$(ISO): kernel.bin
	@mkdir -p isodir/boot/grub
	@cp kernel.bin isodir/boot/kernel
	@cp boot/grub.cfg isodir/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $(ISO) isodir
	@echo "ISO created"

run: $(ISO)
	$(QEMU) -cdrom $(ISO) -serial stdio

clean:
	rm -f $(OBJ) kernel.bin $(ISO)
	rm -rf isodir
