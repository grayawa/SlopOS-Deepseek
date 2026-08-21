#!/usr/bin/env bash
# Capture the QEMU screen via the monitor socket -> build/screen.png
set -e
SOCK=build/qemu-monitor.sock
if [ ! -S "$SOCK" ]; then
    echo "monitor socket not found (is QEMU running?)" >&2
    exit 1
fi
printf 'screendump build/screen.ppm\n' | socat - UNIX-CONNECT:"$SOCK" || true
sleep 0.2
if [ -f build/screen.ppm ]; then
    convert build/screen.ppm build/screen.png 2>/dev/null || cp build/screen.ppm build/screen.png
    echo "captured build/screen.png"
else
    echo "screendump failed" >&2
    exit 1
fi
