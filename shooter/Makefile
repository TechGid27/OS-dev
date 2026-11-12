# DUKE: FOLKLORE SHOOTER - bare-metal NASM build
NASM  = nasm
QEMU  = qemu-system-i386
# Windows has no "python3" (the Store alias is a stub) and uses DirectSound
ifeq ($(OS),Windows_NT)
PY    ?= python
AUDIO ?= dsound
else
PY    ?= python3
AUDIO ?= pa
endif
# QEMU audio backend: pa (Linux PulseAudio), dsound (Windows), none
IMG   = shooter.img
INCS  = $(wildcard *.inc level_src.txt stage*_src.txt tools/genlevel.py)
# lazy (=) so it is evaluated only after game.bin is built; $(shell) runs without sh
PAYLOAD_SECTORS = $(shell $(PY) tools/mkimg.py sectors game.bin)
STAGES = level_src.txt stage2_src.txt stage3_src.txt stage4_src.txt stage5_src.txt

all: $(IMG)

level.inc: $(STAGES) tools/genlevel.py
	$(PY) tools/genlevel.py $@ $(STAGES)

# tools/sprites_gen.py and tools/bg.bin are committed; regenerate only with
# `make extract` (needs ~/Downloads/stitch_pixel_art_asset_extractor)
extract:
	$(PY) tools/extract.py

art.inc: tools/genart.py tools/sprites_gen.py
	$(PY) tools/genart.py $@

game.bin: game.asm $(INCS) level.inc art.inc tools/bg.bin
	$(NASM) -f bin game.asm -o game.bin
	@$(PY) tools/mkimg.py checkgame game.bin

boot.bin: boot.asm game.bin
	$(NASM) -f bin -DPAYLOAD_SECTORS=$(PAYLOAD_SECTORS) boot.asm -o boot.bin
	@$(PY) tools/mkimg.py checkboot boot.bin

$(IMG): boot.bin game.bin
	$(PY) tools/mkimg.py image $(IMG) boot.bin game.bin

run: $(IMG)
	$(QEMU) -drive file=$(IMG),format=raw,if=floppy,index=0 -boot a -audiodev $(AUDIO),id=snd0 -machine pcspk-audiodev=snd0

# headless smoke test: start game (Enter), move right, screenshot (>= 3 colors)
smoke: $(IMG)
	$(PY) tools/smoke.py $(QEMU) $(IMG)
	$(PY) tools/checkppm.py images/smoke.ppm

clean:
	$(PY) tools/mkimg.py clean boot.bin game.bin $(IMG) images/smoke.ppm

.PHONY: all run smoke clean
# never leave a half-written boot.bin/shooter.img behind after a failed build
.DELETE_ON_ERROR:
