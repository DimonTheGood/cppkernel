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
- **Custom GDT.** A hand-built Global Descriptor Table (null, kernel code, kernel data — flat memory model), replacing GRUB's temporary one, loaded via `lgdt` with a proper segment register reload (`ljmp` + `mov`).
- **Custom IDT — full 256-entry table.** All 32 CPU exception vectors wired up through generated ASM stubs (error-code and no-error-code variants) funneling into a single common dispatcher, which decodes the vector number off the stack and calls into a C++ handler. Remaining 224 vectors filled with a default handler so the table has no gaps. Verified live against a real division-by-zero fault (no more silent triple faults / QEMU resets on unhandled exceptions).
- **PIC remap + PIT timer.** The 8259 PIC is remapped so hardware IRQs land on vectors 32–47, clear of CPU exceptions. The PIT (channel 0) is programmed for a steady 100 Hz tick, wired into the IDT dispatcher on IRQ0 (vector 32) with proper End-Of-Interrupt handling — confirmed by a continuous, uninterrupted tick counter in the serial log.

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
  gdt.hpp/.cpp       — Global Descriptor Table
  idt.hpp/.cpp       — Interrupt Descriptor Table, exception dispatcher
  isr.S              — generated ISR stubs + common interrupt entry point
  ports.hpp/.cpp     — shared outb/inb port I/O primitives
  pic.hpp/.cpp       — 8259 PIC remap
  pit.hpp/.cpp       — 8253/8254 PIT (timer) configuration
linker.ld            — 32-bit ELF layout, section placement
Makefile
```

## Roadmap

- [x] **Boot & Multiboot2** — custom header, ASM bootstrap, magic number validation.
- [x] **Serial (COM1) driver** — UART init, string and hex output.
- [x] **VGA text mode output**
- [x] **Multiboot2 info parsing** — tag traversal with bounds checking.
- [x] **Memory map parsing** — reading and printing physical memory regions.
- [x] **Physical Frame Allocator** — bitmap-based, 4KB frames.
- [x] **Basic kernel library** — `memset`, `memcpy`, `memmove`, `memcmp`, `strlen`.
- [x] **GDT** — a proper Global Descriptor Table, replacing GRUB's temporary one, laying groundwork for future user-mode segments.
- [x] **IDT + CPU exceptions** — interrupt descriptor table, all 32 exception handlers via a common dispatcher, default handler for unused vectors.
- [x] **PIC + PIT timer** — remapped interrupt controller, a programmable timer as the tick source for the future scheduler.
- [ ] **Keyboard driver** — IRQ1 handling, scan code to character translation.
- [ ] **Paging** — enabling virtual memory, page directory/tables built on top of the existing frame allocator.
- [ ] **Kernel heap** — `kmalloc`/`kfree` on top of paging.
- [ ] **Threads / scheduler** — context switching driven by PIT ticks, round-robin.
- [ ] **Kernel shell** — an interactive demo shell exercising all implemented subsystems.

## Why a 32-bit kernel

The project intentionally targets i386 (Protected Mode, no Long Mode) — this significantly simplifies the early stages (GDT, paging) and is sufficient for a learning project. A real 64-bit port would require a separate 32-bit stub that sets up page tables and switches the CPU into Long Mode itself before jumping into 64-bit code — GRUB does not do this automatically.