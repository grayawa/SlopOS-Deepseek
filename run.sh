#!/usr/bin/env bash
# Launch SlopOS in QEMU (headless). Serial goes to build/serial.log,
# a QEMU monitor socket is exposed for screendump / input injection.
set -e
mkdir -p build
rm -f build/serial.log build/qemu-monitor.sock

exec qemu-system-x86_64 \
    -cdrom build/slopos.iso \
    -m 512M \
    -vga std \
    -cpu qemu64 \
    -serial file:build/serial.log \
    -monitor unix:build/qemu-monitor.sock,server,nowait \
    -display none \
    -no-reboot \
    "$@"
