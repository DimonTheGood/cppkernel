# My C++ Kernel

A learning project: a 32-bit (i386) kernel written from scratch in C++ and assembly, booted via GRUB using the Multiboot2 protocol. The goal is to understand and implement, by hand, the core subsystems of an operating system — from boot to a basic task scheduler.

## Current features

- **GRUB boot (Multiboot2).** Custom Multiboot2 header, ASM bootstrap with its own stack, transfer into `kmain` with bootloader magic number validation.
- **Serial (COM1) output.** Full UART initialization, string output and hex formatting (32- and 64-bit) — the primary debugging channel at this stage, since it doesn't depend on any later subsystem (unlike VGA, which needs working memory access).
- **VGA text mode.** Basic text output in 80×25 text mode.
- **Multiboot2 info parsing.** Full traversal of the tags passed by the bootloader (`mbi_addr`, `total_size`), with bounds and alignment checks.
- **Memory map.** Reading the physical memory map from GRUB, printing every region (address, length, type) over serial.
- **Physical Frame Allocator.** Bitmap-based physical frame allocator (4KB frames): correct initialization marking available/reserved regions, protection against overflow when handling addresses above 4GB, `alloc_frame()` / `free_frame()`.
- **Basic kernel library.** Freestanding implementations of `memset`, `memcpy`, `memmove`, `memcmp`, `strlen` — no libc dependency.

## Build & run

```sh
make          # builds kernel.iso
make run      # builds (if needed) and runs it in QEMU
```

Requirements: `x86_64-linux-gnu-g++`/`ld` with `-m32` support (`g++-multilib`/`gcc-multilib`), `grub-mkrescue` (`grub-pc-bin`, `xorriso`, `mtools`), `qemu-system-x86_64`.

## Project structure

```
boot/
  grub.cfg          — bootloader configuration
src/
  boot.S             — ASM bootstrap, stack setup, jump into kmain
  kernel.cpp         — entry point, Multiboot2 parsing, subsystem init
  multiboot2.hpp     — Multiboot2 protocol structures and constants
  serial.hpp/.cpp    — COM1 driver
  frame_allocator.hpp/.cpp — bitmap physical frame allocator
  libc.hpp/.cpp      — freestanding kernel library
linker.ld            — 32-bit ELF layout, section placement
Makefile
```

## Roadmap

- [ ] **IDT + CPU exceptions** — interrupt descriptor table, exception handlers (division by zero, page fault, general protection fault, etc.).
- [ ] **PIC + PIT timer** — remapping the interrupt controller, a programmable timer as the tick source for the future scheduler.
- [ ] **Keyboard driver** — IRQ1 handling, scan code to character translation.
- [ ] **Paging** — enabling virtual memory, page directory/tables built on top of the existing frame allocator.
- [ ] **Kernel heap** — `kmalloc`/`kfree` on top of paging.
- [ ] **Threads / scheduler** — context switching driven by PIT ticks, round-robin.
- [ ] **Kernel shell** — an interactive demo shell exercising all implemented subsystems.

## Why a 32-bit kernel

The project intentionally targets i386 (Protected Mode, no Long Mode) — this significantly simplifies the early stages (GDT, paging) and is sufficient for a learning project. A real 64-bit port would require a separate 32-bit stub that sets up page tables and switches the CPU into Long Mode itself before jumping into 64-bit code — GRUB does not do this automatically.
