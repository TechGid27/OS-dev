#!/usr/bin/env python3
"""Check a PPM screenshot from QEMU smoke test: not blank, has expected colors."""
import sys

def main():
    if len(sys.argv) < 2:
        print("usage: checkppm.py file.ppm")
        return 1
    path = sys.argv[1]
    with open(path, "rb") as f:
        data = f.read()
    # PPM P6 header: P6\n<width> <height>\n<maxval>\n
    parts = data.split(b"\n", 3)
    if parts[0].strip() != b"P6":
        print(f"FAIL: not a P6 ppm ({parts[0]!r})")
        return 1
    w, h = map(int, parts[1].split())
    pixels = parts[3]
    n = len(pixels) // 3
    colors = set()
    black = 0
    for i in range(0, n * 3, 3):
        c = (pixels[i], pixels[i + 1], pixels[i + 2])
        colors.add(c)
        if c == (0, 0, 0):
            black += 1
    print(f"size={w}x{h} unique_colors={len(colors)} black_ratio={black / n:.2%}")
    if len(colors) < 3:
        print(f"FAIL: too few colors ({len(colors)}) - screen likely blank or input not rendered")
        return 1
    print("PASS: screenshot has content")
    return 0

if __name__ == "__main__":
    sys.exit(main())
