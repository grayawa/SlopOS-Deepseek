# SlopOS

A from-scratch x86-64 operating system with a graphical desktop, window manager,
keyboard and mouse input, and memory management.

## Features (Implemented)

- **Booting**: Limine v12.5.2 bootloader on BIOS and UEFI
- **Graphics**: Framebuffer-based display with 8x16 font rendering
- **Window Manager**: Windows with title bars, close buttons, dragging, focus
- **Input**: PS/2 keyboard (scancode set 1) with key buffer, PS/2 mouse (3-button + scroll)
- **Interrupts**: GDT, IDT with 48 ISR stubs, PIC remapping (IRQ 0-15 mapped to 0x20-0x2F)
- **Memory**: Bitmap-based physical page allocator (4KB pages), detects usable RAM via Limine memmap
- **Debug**: Serial output on COM1 (115200 8N1)
- **Processes**: Process control block infrastructure, context switching (WIP)

## Features (Planned)

- Preemptive multitasking scheduler
- Virtual memory management with user/kernel separation
- System call interface with Linux x86-64 ABI compatibility
- Virtual filesystem with tmpfs
- ELF executable loader
- Terminal emulator with ANSI escape support
- Unix-like /proc, /dev filesystems

## Building

Requirements:
- GCC 14+ (x86-64 host or cross-compiler)
- GNU Make
- xorriso
- Limine bootloader binaries (v12.5.2+) in `limine-binary/`

```sh
make          # Build kernel ELF (build/slopos.elf)
make iso      # Build bootable ISO (slopos.iso)
make run      # Run in QEMU with serial console output
make run-gui  # Run in QEMU with graphics display
make clean    # Remove build artifacts
```

## Architecture

- **Target**: x86-64, higher-half kernel at 0xFFFFFFFF80000000
- **Boot protocol**: Limine v1 (tagged requests/responses)
- **Code model**: kernel (-mcmodel=kernel, -mno-red-zone)
- **Page size**: 4KB

## License

All original SlopOS code: **0BSD** (Zero-Clause BSD License)

Third-party dependencies:
- **Limine bootloader** (BSD-2-Clause) - https://github.com/limine-bootloader/limine

## Boot Screenshot

On boot, SlopOS displays the logo, system information, memory map, and a
graphical desktop with window manager. Keyboard and mouse input are available
for interacting with terminal windows.
