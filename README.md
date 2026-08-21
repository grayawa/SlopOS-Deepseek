# SlopOS

SlopOS is a from-scratch x86-64 operating system that boots in QEMU. It is
written independently (no copied kernel, distribution, or ported OS) and
released under the 0BSD license.

## What it does

* Boots in QEMU via GRUB/multiboot2 into 64-bit long mode.
* Renders a graphical desktop with a framebuffer.
* Runs an interactive window manager (draggable windows, close buttons,
  taskbar, mouse and keyboard input).
* Provides a terminal with a shell.
* Runs user programs in ring 3 (separate address spaces) through the
  Linux x86-64 syscall ABI.
* Loads and runs static Linux x86-64 ELF executables.

## Quick start

```
make            # builds build/slopos.iso (kernel + hello + primes programs)
./run.sh        # boots it in QEMU (headless; serial -> build/serial.log)
./shot.sh       # captures build/screen.png via the QEMU monitor
```

In the terminal shell, type `help` for built-in commands, or `run hello` /
`run primes` to execute the bundled user programs.

## Documentation

* [docs/STATUS.md](docs/STATUS.md) — honest account of implemented and
  missing functionality.
* [docs/BUILD.md](docs/BUILD.md) — build and run instructions.
* [LICENSE](LICENSE) — 0BSD.

## Goals

* Boot in QEMU with a graphical framebuffer.
* Interactive graphical desktop: windows, keyboard and mouse input.
* Memory management, interrupts, processes, files, executable loading,
  input, graphics, and user programs.
* Compatibility with the Linux x86-64 userspace ABI, targeting unmodified
  Linux executables. Partial compatibility is clearly distinguished from
  general compatibility and is verified by running unchanged binaries.

## License

0BSD (see LICENSE).
