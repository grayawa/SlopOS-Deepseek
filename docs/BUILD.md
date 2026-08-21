# SlopOS build and run instructions

## Requirements

* Linux x86-64 host (developed on AOSC Linux)
* QEMU (`qemu-system-x86_64`)
* GCC / binutils (for the kernel and the user programs)
* GRUB 2 (`grub-mkrescue`, `xorriso`, `mtools`)
* Python 3 (generates the bitmap font and ISR table)

## Building

```
make          # builds build/slopos.iso (kernel + hello user program)
```

The build:
1. Compiles the kernel objects (`src/*.c`, `src/*.S`) into `build/slopos.elf`.
2. Compiles `programs/hello.c` into a static Linux-ABI ELF (`build/hello.elf`).
3. Assembles a bootable ISO with GRUB, using `grub/grub.cfg`.

The kernel, font, and ISR table are generated from `tools/genfont.py` and
`tools/genisr.py`; run `make gen` to regenerate them.

## Running in QEMU

```
./run.sh       # boots build/slopos.iso (headless, serial -> build/serial.log)
```

`run.sh` launches QEMU with:
* a serial console on `build/serial.log`,
* a QEMU monitor socket at `build/qemu-monitor.sock`,
* a VGA framebuffer (1024x768x32 via VBE).

To interact with the running desktop, inject input via the QEMU monitor:

```
printf 'sendkey h\n' | socat - UNIX-CONNECT:build/qemu-monitor.sock
printf 'mouse_move 100 100\n' | socat - UNIX-CONNECT:build/qemu-monitor.sock
```

To capture a screenshot of the graphical desktop:

```
./shot.sh       # writes build/screen.png
```

## Testing the user program

At boot, SlopOS runs the `hello` user program once. You can also run it from
the terminal shell by typing:

```
run hello
```

The program executes in ring 3, issues Linux `write()` and `exit()` syscalls,
and its output appears in the terminal window.

## Clean

```
make clean     # removes build/
```
