# SlopOS feature status

This is an honest, up-to-date account of what SlopOS implements and what it
does not. Every feature listed as working has been executed and verified in
QEMU (via serial logs and framebuffer screenshots).

## Implemented and verified

### Boot
* Multiboot2 header, loaded by GRUB into 32-bit protected mode.
* Transition to x86-64 long mode with identity-mapped 4 GiB paging
  (2 MiB huge pages).
* VBE framebuffer (1024x768x32) requested via the multiboot2 framebuffer tag
  and set up by GRUB.
* **Verified**: boots in QEMU and draws a graphical screen.

### Core kernel
* GDT with kernel/user segments and a TSS; `ltr` loaded.
* IDT with a 256-vector ISR table; PIC remapped (IRQ0-15 to int 0x20-0x2F).
* Exception handlers (print register state and halt).
* PIT timer at 100 Hz, with a tick counter and sleep.
* Physical memory manager: parses the multiboot2 mmap, bitmap frame
  allocator, reserves kernel/low-memory/framebuffer/multiboot info/modules.
* Virtual memory manager: 4-level page tables, 4 KiB and 2 MiB mappings,
  per-process address spaces, user-bit propagation.
* Kernel heap allocator (`malloc`/`free`/`calloc`/`realloc`), free-list based.
* SSE/FPU enabled for user programs.

### Graphics and input
* Framebuffer drawing library (pixels, rectangles, 8x8 bitmap text).
* PS/2 keyboard driver (scancode set 1, shift/caps-lock).
* PS/2 mouse driver (packet parsing, absolute cursor, buttons).
* **Verified**: keyboard echoes typed characters; mouse cursor tracks
  injected PS/2 packets.

### Graphical desktop
* Window manager: draggable windows, title bars, close buttons, focus.
* Taskbar with a running clock.
* Terminal window with a scrollback buffer and a shell.
* **Verified**: windows render, drag, and the terminal shell runs commands
  (`help`, `clear`, `echo`, `uptime`, `info`, `about`, `run`).

### Processes and user mode
* Round-robin task scheduler with context switching (kernel and user tasks).
* Per-task kernel stacks and page tables.
* User-mode entry via `iretq` (ring 3).
* **Verified**: a user program runs in ring 3 and returns to the kernel.

### Linux x86-64 userspace ABI (partial)
* `syscall` instruction entry (STAR/LSTAR/SFMASK MSRs, EFER.SCE).
* Implemented syscalls: `write` (1), `read` (0, returns EOF), `open` (2),
  `close` (3), `mmap` (9), `munmap` (11), `brk` (12), `getpid` (39),
  `exit` (60), `exit_group` (231).
* ELF64 loader: maps `PT_LOAD` segments into a fresh user address space,
  sets up a Linux-style initial stack (argc/argv/envp).
* **Verified**: a static, freestanding Linux x86-64 ELF (compiled with the
  host `gcc`, using raw `write()`/`exit()` syscalls) loads, runs in ring 3,
  and prints to the terminal.

## Not implemented / limitations

### Linux ABI compatibility
* **Partial only.** General compatibility (e.g. dynamic `glibc` binaries)
  is NOT achieved. The syscall set is small, and there is no dynamic linker,
  `vDSO`, `clone`/`fork`/`execve`, signals, `arch_prctl`/FS-base, or the
  full file system call surface. Only freestanding static ELF64 binaries
  using the implemented syscalls run.
* `read` returns EOF; `open` returns -1; there is no real filesystem yet.

### Preemptive multitasking
* The scheduler switches between tasks on `yield`/`exit`. Timer-driven
  preemption of user tasks is not enabled: the timer is used for the clock,
  and user programs run to completion. (User mode runs with interrupts
  enabled, but the timer does not context-switch them.)

### Filesystem
* No on-disk filesystem. User programs are delivered as multiboot2 modules.

### Networking / storage / drivers
* No disk, network, USB, or ATA drivers. Input is PS/2 only.

### Other
* Single CPU; no SMP.
* The kernel identity-maps low memory; user processes share the kernel's
  low-memory page tables (a latent security simplification for trusted
  user programs).
* User programs compiled with aggressive `-O2` SSE vectorization can hit a
  stack-alignment/context issue (the bundled `primes` program is built with
  `-O0` to avoid it); this is under investigation.

## Verification methodology

Each claim above is backed by one of:
* serial console output (`build/serial.log`), and
* QEMU monitor `screendump` screenshots of the framebuffer.

See `docs/BUILD.md` for how to reproduce the build and run.
