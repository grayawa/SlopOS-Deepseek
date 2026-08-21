#!/usr/bin/env bash
# Launch SlopOS in QEMU with a visible GUI window.
#
# Once the window opens, click on it to capture the keyboard and mouse
# (QEMU grabs input). To release the grab, press Ctrl+Alt+G.
#
# If the GTK backend is unavailable, replace '-display gtk' with '-display sdl'.
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
    -display gtk \
    -no-reboot \
    "$@"
