#!/usr/bin/env python3
"""Record SlopOS interaction (typing + mouse) to a video via QEMU screendumps.

Usage: record.py <output.mp4>
Requires a running QEMU (./run.sh) with a monitor socket at build/qemu-monitor.sock.

Captures at a fixed frame rate and injects scripted input on a timeline.
"""
import os, subprocess, sys, time

SOCK = "build/qemu-monitor.sock"
REC = "build/rec"
FPS = 15
os.makedirs(REC, exist_ok=True)
for f in os.listdir(REC):
    os.remove(os.path.join(REC, f))

def mon(cmd):
    subprocess.run(["socat", "-", "UNIX-CONNECT:" + SOCK],
                   input=(cmd + "\n").encode(), stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)

frames = 0
def cap():
    global frames
    mon("screendump %s/frame%04d.ppm" % (REC, frames))
    frames += 1

def key(k): mon("sendkey " + k)
def keys(s):
    for ch in s:
        key("spc" if ch == " " else ch)
        time.sleep(0.05)
def mouse(dx, dy): mon("mouse_move %d %d" % (dx, dy))
def btn(b): mon("mouse_button %d" % b)

# --- timeline of (time, action) ---
EVENTS = [
    (0.0,  lambda: None),                       # boot desktop
    (1.0,  lambda: keys("help")),               # type help
    (2.4,  lambda: key("ret")),                 # submit help
    (4.0,  lambda: keys("run primes")),         # type run primes
    (5.6,  lambda: key("ret")),                 # submit run primes
    (8.0,  lambda: keys("run cat")),            # type run cat
    (9.6,  lambda: key("ret")),                 # submit run cat
    (11.5, lambda: mouse(200, -150)),           # move cursor up-right
    (12.5, lambda: mouse(-150, 100)),           # move cursor back toward center
    (13.5, lambda: mouse(188, -314)),           # position over the About title bar
    (14.3, lambda: btn(1)),                     # press left button (start drag)
    (15.3, lambda: mouse(120, 60)),             # drag the window right/down
    (16.3, lambda: btn(0)),                     # release
    (17.5, lambda: keys("echo recorded")),      # type echo
    (19.0, lambda: key("ret")),                 # submit echo
]
DURATION = 20.5

start = time.time()
ei = 0
while time.time() - start < DURATION:
    t = time.time() - start
    while ei < len(EVENTS) and t >= EVENTS[ei][0]:
        EVENTS[ei][1]()
        ei += 1
    cap()
    time.sleep(1.0 / FPS)

print("captured %d frames" % frames)

out = sys.argv[1] if len(sys.argv) > 1 else "build/slopos.mp4"
subprocess.run([
    "ffmpeg", "-y", "-loglevel", "error",
    "-framerate", str(FPS),
    "-i", REC + "/frame%04d.ppm",
    "-c:v", "libx264", "-pix_fmt", "yuv420p", "-movflags", "+faststart",
    out,
])
print("wrote %s (%d fps, %d frames)" % (out, FPS, frames))
