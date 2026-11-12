#!/usr/bin/env python3
"""mkos.py - write os.img, a 1.44 MB floppy image.

  LBA 0        boot.bin       (boot sector)
  LBA 1-16     stage2.bin     (real-mode loader, 8 KB max)
  LBA 17       directory      (16 entries x 32 bytes)
  LBA 18...    file data, one contiguous run per file

Every file has a fixed load address in RAM, so stage2 can put it in place
without a filesystem driver:
  KERNEL.BIN  0x60000   kernel (max 64 KB)
  DUKE.BIN    0x10000   shooter game (DUKE.BIN = shooter/game.bin)
  HELLO.TXT   0x50000   text
  MUSIC.SPK   0x51000   music data
  PIC.IMG     0x80000   picture, 64000 bytes
  CLIP.VID    0x90000   video frames (game stack starts at 0x9FF00)
"""
import math
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
IMG_SIZE = 1474560
SECTOR = 512
STAGE2_SECTORS = 16
DIR_LBA = 17
DATA_LBA = 18

# name (8.3, uppercase), file, load address, max size
FILES = [
    ("KERNEL.BIN", "kernel.bin", 0x60000, 0x10000),
    ("DUKE.BIN", "../shooter/game.bin", 0x10000, 0x30000),
    ("HELLO.TXT", "hello.txt", 0x50000, 0x1000),
    ("MUSIC.SPK", "music.spk", 0x51000, 0xF000),
    ("PIC.IMG", "pic.bin", 0x80000, 0x10000),
    ("CLIP.VID", "clip.bin", 0x90000, 0xF000),
]


def read(path):
    with open(os.path.join(HERE, path), "rb") as f:
        return f.read()


def main():
    boot = read("boot.bin")
    if len(boot) != SECTOR:
        sys.exit(f"boot.bin must be 512 bytes, got {len(boot)}")
    stage2 = read("stage2.bin")
    if len(stage2) > STAGE2_SECTORS * SECTOR:
        sys.exit("stage2.bin does not fit in 16 sectors")

    img = bytearray(IMG_SIZE)
    img[0:SECTOR] = boot
    img[SECTOR:SECTOR + len(stage2)] = stage2

    directory = bytearray(SECTOR)
    lba = DATA_LBA
    for i, (name, path, load, max_size) in enumerate(FILES):
        data = read(path)
        if len(data) > max_size:
            sys.exit(f"{name}: {len(data)} bytes > max {max_size}")
        sectors = math.ceil(len(data) / SECTOR)
        start = lba * SECTOR
        img[start:start + len(data)] = data
        entry = struct.pack("<12sIIII4x", name.encode("ascii"), lba,
                            sectors, load, len(data))
        directory[i * 32:(i + 1) * 32] = entry
        print(f"{name:10s} lba {lba:4d} sectors {sectors:4d} "
              f"load 0x{load:05X} size {len(data)}")
        lba += sectors
    if lba > IMG_SIZE // SECTOR:
        sys.exit("files do not fit on the floppy")

    img[DIR_LBA * SECTOR:(DIR_LBA + 1) * SECTOR] = directory
    with open(os.path.join(HERE, "os.img"), "wb") as f:
        f.write(img)
    print(f"os.img: {IMG_SIZE} bytes, {lba} sectors used")


if __name__ == "__main__":
    main()
