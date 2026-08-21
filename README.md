# SlopOS

SlopOS is a from-scratch x86-64 operating system that boots in QEMU. It is
written independently (no copied kernel, distribution, or ported OS) and
released under the 0BSD license.

See [docs/STATUS.md](docs/STATUS.md) for an honest, up-to-date account of what
is implemented and what is missing, and [docs/BUILD.md](docs/BUILD.md) for
build and run instructions.

## Quick start

```
make            # builds build/slopos.iso
./run.sh        # boots it in QEMU (headless; serial -> build/serial.log)
./shot.sh       # captures build/screen.png via the QEMU monitor
```

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
