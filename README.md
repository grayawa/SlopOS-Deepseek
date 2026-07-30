# SlopOS

A from-scratch x86-64 operating system with a graphical desktop, written for learning and experimentation.

## Features (Implemented)

- Booting via Limine v12.5.2 bootloader
- Graphical framebuffer with font rendering
- GDT, IDT, interrupt handling (PIC)
- PS/2 keyboard driver with key buffer
- Physical memory manager (bitmap allocator)
- Serial debug output (COM1)
- Memory map display on boot

## Features (Planned)

- Window manager and GUI toolkit
- Terminal emulator
- Mouse driver
- Virtual memory management with paging
- Process/task scheduler and syscalls
- Virtual filesystem with tmpfs
- ELF executable loader
- Linux x86-64 ABI compatibility layer

## Building

Requirements:
- GCC (x86-64)
- GNU Make
- xorriso
- Limine bootloader binaries (v12.5.2+) in `limine-binary/`

```sh
make          # Build kernel ELF
make iso      # Build bootable ISO
make run      # Run in QEMU with serial output
make run-gui  # Run in QEMU with graphics
```

## License

All original SlopOS code: 0BSD (Zero-Clause BSD)

Third-party dependencies:
- Limine bootloader (BSD-2-Clause) - https://github.com/limine-bootloader/limine

## Architecture

Target: x86-64, booting via Limine protocol on BIOS/UEFI.
Kernel is linked at 0xFFFFFFFF80000000 (higher half).
