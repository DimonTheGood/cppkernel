# cppkernel

A small 32-bit (i386) kernel written from scratch in C++ and assembly, booted by GRUB via Multiboot2.

Features: GDT/IDT, interrupts, PIC/PIT timer, physical frame allocator, paging, kernel heap (`kmalloc`/`kfree`, `new`/`delete`), preemptive round-robin kernel threads, PS/2 keyboard, VGA console and an interactive shell.

## Install and run (Ubuntu/Debian)

```sh
sudo apt install git make g++ g++-multilib grub-common grub-pc-bin xorriso mtools qemu-system-x86
git clone https://github.com/DimonTheGood/cppkernel
cd cppkernel
make run
```

QEMU opens with the kernel shell; type `help` to list commands.

