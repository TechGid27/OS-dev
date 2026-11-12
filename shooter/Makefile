# DUKE: FOLKLORE SHOOTER - bare-metal NASM build
NASM  = nasm
QEMU  = qemu-system-i386
PY    = python3
IMG   = shooter.img
INCS  = $(wildcard *.inc level_src.txt stage*_src.txt tools/genlevel.py)
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
	@echo "game.bin: $$(stat -c%s game.bin) bytes"
	@if [ $$(stat -c%s game.bin) -gt 196608 ]; then echo "ERROR: game.bin > 192KB (overlaps framebuffer at 0x40000)"; exit 1; fi

boot.bin: boot.asm game.bin
	$(NASM) -f bin -DPAYLOAD_SECTORS=$$($(PY) -c "import math,os;print(math.ceil(os.path.getsize('game.bin')/512))") boot.asm -o boot.bin
	@test $$(stat -c%s boot.bin) -eq 512 || { echo "ERROR: boot.bin must be 512 bytes"; exit 1; }
	@echo "boot.bin: 512 bytes OK"

$(IMG): boot.bin game.bin
	cat boot.bin game.bin > $(IMG)
	truncate -s 1474560 $(IMG)
	@echo "$(IMG): ready"

run: $(IMG)
	$(QEMU) -drive file=$(IMG),format=raw,if=floppy,index=0 -boot a \
	        -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0

# headless smoke test: start game (Enter), move right, screenshot (>= 3 colors)
smoke: $(IMG)
	bash -c '{ sleep 1.5; echo "sendkey ret 1200"; sleep 1.0; echo "sendkey right 400"; sleep 0.8; echo "screendump images/smoke.ppm"; sleep 0.3; echo quit; } | \
	  $(QEMU) -drive file=$(IMG),format=raw,if=floppy,index=0 -boot a \
	          -display none -monitor stdio -no-reboot -no-shutdown'
	$(PY) tools/checkppm.py images/smoke.ppm

clean:
	rm -f boot.bin game.bin $(IMG) images/smoke.ppm

.PHONY: all run smoke clean
