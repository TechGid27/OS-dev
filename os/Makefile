# OS build: boot sector + stage2 loader + kernel + files on a 1.44 MB floppy
NASM = nasm
PY   = python3
AUDIO ?= pa
# QEMU audio backend: pa (Linux PulseAudio), dsound (Windows), none
GAME = ../shooter/game.bin

ASSETS = palette.bin consts.inc font8x16.bin hello.txt music.spk pic.bin clip.bin icons.bin

all: os.img

# assets are committed; regenerate only with `make assets` (needs the
# Downloads photo/video and the kbd font, see mkassets.py)
assets:
	$(PY) mkassets.py

boot.bin: boot.asm
	$(NASM) -f bin boot.asm -o boot.bin

stage2.bin: stage2.asm
	$(NASM) -f bin stage2.asm -o stage2.bin

kernel.bin: kernel.asm consts.inc palette.bin font8x16.bin icons.bin
	$(NASM) -f bin kernel.asm -o kernel.bin

$(GAME):
	$(MAKE) -C ../shooter game.bin

os.img: boot.bin stage2.bin kernel.bin $(ASSETS) $(GAME) mkos.py
	$(PY) mkos.py

run: os.img
	qemu-system-i386 -drive file=os.img,format=raw,if=floppy,index=0 -boot a \
	        -audiodev $(AUDIO),id=snd0 -machine pcspk-audiodev=snd0

clean:
	rm -f boot.bin stage2.bin kernel.bin os.img

.PHONY: all run clean
