#!/usr/bin/env python3
"""Cross-platform build helpers (replace stat/test/cat/truncate in the Makefile).

  mkimg.py sectors game.bin        -> prints ceil(size/512)
  mkimg.py checkgame game.bin      -> prints size, fails if > 192KB
  mkimg.py checkboot boot.bin      -> fails unless exactly 512 bytes
  mkimg.py clean file...           -> delete files if present
  mkimg.py image out.img boot.bin game.bin  -> concat, pad to 1.44MB floppy
"""
import math
import os
import sys

GAME_LIMIT = 196608
FLOPPY = 1474560


def main(argv):
    cmd = argv[1]
    if cmd == "sectors":
        print(math.ceil(os.path.getsize(argv[2]) / 512))
    elif cmd == "checkgame":
        size = os.path.getsize(argv[2])
        print(f"{argv[2]}: {size} bytes")
        if size > GAME_LIMIT:
            print("ERROR: game.bin > 192KB (overlaps framebuffer at 0x40000)")
            return 1
    elif cmd == "checkboot":
        if os.path.getsize(argv[2]) != 512:
            print("ERROR: boot.bin must be 512 bytes")
            return 1
        print("boot.bin: 512 bytes OK")
    elif cmd == "image":
        data = b"".join(open(p, "rb").read() for p in argv[3:])
        with open(argv[2], "wb") as f:
            f.write(data.ljust(FLOPPY, b"\0"))
        print(f"{argv[2]}: ready")
    elif cmd == "clean":
        for p in argv[2:]:
            if os.path.exists(p):
                os.remove(p)
    else:
        print(__doc__)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
