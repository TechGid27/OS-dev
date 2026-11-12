#!/usr/bin/env python3
"""Cross-platform headless smoke test.

usage: smoke.py <qemu> <image>
Starts the game (Enter), moves right, takes a screenshot via the QEMU monitor.
The monitor is reached over a localhost TCP socket (stdio pipes are unreliable
for the QEMU monitor on Windows).
"""
import os
import socket
import subprocess
import sys
import time

PPM = "images/smoke.ppm"


def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def connect(port, proc, timeout=15):
    end = time.time() + timeout
    while time.time() < end:
        if proc.poll() is not None:
            raise RuntimeError("QEMU exited early (is the image/qemu path correct?)")
        try:
            return socket.create_connection(("127.0.0.1", port), timeout=2)
        except OSError:
            time.sleep(0.2)
    raise RuntimeError("could not connect to QEMU monitor")


def main():
    qemu, img = sys.argv[1], sys.argv[2]
    os.makedirs("images", exist_ok=True)
    if os.path.exists(PPM):
        os.remove(PPM)
    port = free_port()
    try:
        p = subprocess.Popen(
            [qemu, "-drive", f"file={img},format=raw,if=floppy,index=0", "-boot", "a",
             "-display", "none", "-no-reboot", "-no-shutdown",
             "-monitor", f"tcp:127.0.0.1:{port},server,nowait"],
            stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except FileNotFoundError:
        print(f"ERROR: cannot run '{qemu}' - add QEMU to PATH or pass QEMU=<full path>")
        return 1
    try:
        sock = connect(port, p)

        def cmd(text, wait):
            sock.sendall((text + "\n").encode())
            time.sleep(wait)

        time.sleep(1.5)
        cmd("sendkey ret 1200", 1.0)
        cmd("sendkey right 400", 0.8)
        cmd("screendump " + PPM, 0.3)
        for _ in range(20):  # wait for the dump to be written
            if os.path.exists(PPM) and os.path.getsize(PPM) > 0:
                break
            time.sleep(0.25)
        try:
            cmd("quit", 0)
        except OSError:
            pass
        sock.close()
    except RuntimeError as e:
        print("ERROR:", e)
        return 1
    finally:
        try:
            p.wait(timeout=5)
        except subprocess.TimeoutExpired:
            p.kill()
    return 0 if os.path.exists(PPM) else 1


if __name__ == "__main__":
    sys.exit(main())
