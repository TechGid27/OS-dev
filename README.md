# OS dev: a small bare-metal OS and the DUKE folklore shooter

Two projects in one repo:

- **`os/`**: a small x86 operating system. It boots from a 1.44 MB floppy image,
  shows a file browser, and can open a text file, a picture, play a melody, play a
  video, and launch the game.
- **`shooter/`**: DUKE: FOLKLORE SHOOTER, a 2D platformer written in NASM. It is
  also a standalone floppy image, and it is launched from the OS as `DUKE.BIN`.

Everything is written in NASM (16-bit and 32-bit x86), with Python 3 build tools.
No external libraries are used at runtime.

---

## Requirements

| Tool | Needed for |
| --- | --- |
| `nasm` | assembling the OS and the game (always) |
| `make` | the build targets (always) |
| `python3` (`python` on Windows) | build scripts: `genlevel.py`, `genart.py`, `mkos.py` (always) |
| `qemu-system-i386` | `make run`, `make smoke` |
| sound backend | `make run` uses `AUDIO=pa` (PulseAudio) by default. Windows: `AUDIO=dsound`. No sound: `AUDIO=none`. |
| Python `numpy`, `Pillow`, `scipy` | only `make assets` (OS) and `make extract` (shooter) |
| `ffmpeg`, `ffprobe` | only `make assets`, to extract video frames |

Normal build and run (after a fresh clone) needs only `nasm`, `make`, `python3`,
and `qemu-system-i386`. The generated assets are committed, so nothing is
regenerated from the Downloads folder.

---

## Quick start

### Linux (Fedora, Ubuntu, Debian)

Install the tools:

```sh
# Fedora
sudo dnf install nasm qemu-system-x86 make python3
# Ubuntu / Debian
sudo apt install nasm qemu-system-x86 make python3
```

Build and run:

```sh
cd "OS dev/os"
make            # builds os.img (also builds the game)
make run        # starts the OS in QEMU with PulseAudio sound

cd "OS dev/shooter"
make            # builds shooter.img, the game as a standalone floppy image
make run        # starts the game alone in QEMU
make smoke      # headless test: starts the game, takes a screenshot, checks it
```

### Windows 10 / 11

The Makefiles use Unix tools (`cat`, `truncate`, `stat`, `rm`, `bash`). Use
**Git Bash** (installed with Git for Windows) as the terminal.

1. **Git for Windows**: <https://git-scm.com/download/win>. This gives Git Bash.
2. **NASM**: <https://www.nasm.us>. Add it to `PATH`.
3. **QEMU for Windows**: <https://www.qemu.org/download/#windows>. Add the install
   folder (the one with `qemu-system-i386.exe`) to `PATH`.
4. **Python 3**: <https://www.python.org/downloads/>. Tick "Add python.exe to PATH".
5. **GNU Make**: `choco install make` (Chocolatey), or install `make` from MSYS2.

Open Git Bash in the `OS dev` folder, then:

```sh
cd "os"
make PY=python AUDIO=dsound    # build os.img
make run PY=python AUDIO=dsound

cd "../shooter"
make PY=python AUDIO=dsound    # build shooter.img
make run PY=python AUDIO=dsound
make smoke PY=python
```

`PY=python` is needed because Windows usually calls Python `python`, not `python3`.
`AUDIO=dsound` is the Windows sound backend. Use `AUDIO=none` to run without sound.

---

## Directory structure

```
OS dev/
├── README.md              this file
├── .gitignore             ignores build outputs (os.img, game.bin, ...)
├── assets/                source media used to make the OS assets
│   ├── pic_source.jpeg        photo  -> PIC.IMG
│   ├── clip_source.mp4        video  -> CLIP.VID
│   ├── default8x16.psfu.gz    console font -> font8x16.bin
│   └── stitch_pixel_art_asset_extractor/   AI-generated sprite sheets (shooter)
├── os/                    the operating system
│   ├── Makefile           build targets: all, assets, run, clean
│   ├── boot.asm           512-byte boot sector, loads stage2
│   ├── stage2.asm         real-mode loader: reads the disk, loads files, enters protected mode
│   ├── kernel.asm         kernel: interrupts, file browser, viewers, shutdown
│   ├── mkassets.py        makes the assets below from assets/ (needs `make assets`)
│   ├── mkos.py            writes os.img: boot, stage2, directory, file data
│   ├── palette.bin        64-colour VGA palette (6-bit RGB), from shooter/video.inc
│   ├── consts.inc         palette indices of UI colours (generated)
│   ├── font8x16.bin       256 glyphs, 8x16 (generated)
│   ├── icons.bin          six 16x16 menu icons (generated)
│   ├── pic.bin            PIC.IMG, 320x200 palette indices (generated)
│   ├── clip.bin           CLIP.VID, 14 frames of 80x50 (generated)
│   ├── music.spk          MUSIC.SPK, melody as (PIT divisor, ticks) pairs (generated)
│   └── hello.txt          HELLO.TXT, demo text (generated)
└── shooter/               DUKE: FOLKLORE SHOOTER (the game)
    ├── Makefile           build targets: all, game.bin, run, smoke, extract, clean
    ├── boot.asm           standalone boot sector (loads game.bin at 0x10000)
    ├── game.asm           entry point, main loop, game states, progression
    ├── level_src.txt      stage 1 map (text, one char per tile)
    ├── stage2_src.txt     stage 2 map
    ├── stage3_src.txt     stage 3 map
    ├── stage4_src.txt     stage 4 map
    ├── stage5_src.txt     stage 5 map
    ├── level.inc          generated: all stage maps and spawn lists
    ├── art.inc            generated: sprites, tiles, font (from tools/genart.py)
    ├── video.inc          VGA mode 13h: palette, fill, flip, backdrop
    ├── tiles.inc          tile map, camera, collision lookup
    ├── physics.inc        gravity, ground check
    ├── player.inc         player state, input, drawing
    ├── enemies.inc        enemy AI: aswang, kapre, tikbalang, manananggal, boss
    ├── bullets.inc        player bullets and enemy fireballs
    ├── pickups.inc        health, ammo, key
    ├── hud.inc            top status bar
    ├── font.inc           text drawing (draw_text, draw_text_2x)
    ├── input.inc          keyboard IRQ1, key_state table
    ├── interrupts.inc     PIC remap, IDT
    ├── timer.inc          PIT 60 Hz tick
    ├── sound.inc          PC speaker sound effects (PIT channel 2)
    ├── images/            screenshots from testing (debug output, not game assets)
    └── tools/
        ├── genart.py      writes art.inc from the sprite definitions
        ├── sprites_gen.py sprite data generated from the sheets (committed)
        ├── genlevel.py    writes level.inc from all stage*_src.txt files
        ├── extract.py     cuts sprites from assets/ sheets (make extract)
        ├── checkppm.py    checks a screenshot has colours (used by make smoke)
        └── bg.bin         background image data (committed)
```

---

## Architecture

### The OS

**Boot chain**

```
BIOS -> boot.asm (LBA 0) -> stage2.asm (LBA 1-16) -> kernel.asm (0x60000, protected mode)
```

1. **`boot.asm`** loads LBA 1-16 to physical `0x8000`, then jumps there with the
   boot drive in `DL`.
2. **`stage2.asm`** (real mode) sets VGA mode 13h, reads the directory at LBA 17
   to `0x0500`, and loads every directory entry to its load address. Then it
   enables A20, loads the GDT, switches to 32-bit protected mode, and jumps to
   `0x60000`.
3. **`kernel.asm`** (protected mode) sets up the IDT (IRQ0 timer, IRQ1 keyboard),
   remaps the PIC, programs the PIT to 100 Hz, loads the palette, and shows the
   file browser.

**Disk layout** (1.44 MB floppy, 512-byte sectors)

| LBA | Contents |
| --- | --- |
| 0 | `boot.bin` |
| 1-16 | `stage2.bin` (8 KB max) |
| 17 | directory: 16 entries of 32 bytes |
| 18+ | file data, one contiguous run per file |

Directory entry (32 bytes): `name[12]` (zero padded), `lba` (dd), `sectors` (dd),
`load address` (dd), `size` (dd), 4 reserved bytes. The kernel finds files by name
in the directory at `0x0500`.

**Memory map**

| Physical address | Used for |
| --- | --- |
| `0x0500` | directory (512 bytes) |
| `0x7C00` | boot sector, real-mode stack |
| `0x7E00` | loader bounce buffer |
| `0x8000` | stage2 (8 KB) |
| `0x10000`-`0x3FFFF` | `DUKE.BIN` (the game, about 194 KB) |
| `0x40000`-`0x4FA00` | game framebuffer (game only) |
| `0x50000` | `HELLO.TXT` |
| `0x51000` | `MUSIC.SPK` |
| `0x60000`-`0x6FFFF` | `KERNEL.BIN` (64 KB max) |
| `0x70000`-`0x7FFF0` | kernel stack (grows down from `0x7FFF0`) |
| `0x80000`-`0x8F9FF` | `PIC.IMG` (64000 bytes) |
| `0x90000`-`0x9DA08` | `CLIP.VID` |
| `0x9FF00` | game stack (grows down) |
| `0xA0000` | VGA memory (mode 13h, 320x200) |

Every file has a fixed load address, set in `mkos.py`. The loader does not need a
filesystem driver.

**Kernel modules** (`kernel.asm`)

- **Interrupts:** IRQ0 counts `ticks` (100 Hz). IRQ1 stores the last make code in `kb_key`.
- **Drawing:** `fill_rect`, `draw_char` (8x16 font, bit 7 is the left pixel), `draw_string`,
  `draw_icon_x2` (16x16 icon drawn at 2x).
- **File browser:** a 3x2 grid of icons with names, and a help list at the bottom.
  Keys O, P, M, V, G, Q open the matching item.
- **Viewers:**
  - *Text* (HELLO.TXT): 38 columns, 10 rows.
  - *Picture* (PIC.IMG): copied to VGA memory. A bar at the bottom shows `B: BACK`.
  - *Music* (MUSIC.SPK): PIT channel 2 plays each note, and port `0x61` turns the speaker on and off.
  - *Video* (CLIP.VID): 14 frames, drawn at 2x (160x100), 300 ms per frame.
  - *Game* (DUKE.BIN): jumps to `0x10000`. ESC inside the game jumps back to `0x60000`.
- **Quit:** ACPI soft-off (ports `0x604`, `0xB004`, `0x4004`). If the machine keeps running,
  it shows "IT IS SAFE TO TURN OFF THE COMPUTER" and halts.

### The game

**Startup and loop.** The game runs in protected mode, sets its own IDT, timer, and
sound, and uses a 320x200 VGA mode 13h screen. It draws to an offscreen framebuffer
at `0x40000` and copies the frame to VGA memory once per tick.

**Game states** (`game.asm`):

| State | Meaning |
| --- | --- |
| `ST_TITLE` | title screen. ENTER starts the game. |
| `ST_PLAY` | playing |
| `ST_DEAD` | death animation, then respawn (or game over) |
| `ST_GAMEOVER` | no lives left. R returns to the title. |
| `ST_CLEAR` | stage clear. ENTER goes to the next stage, or after stage 5, R returns to the title. |

**Map data.** Each stage is a text file with one character per tile:

| Char | Meaning |
| --- | --- |
| `#` | solid ground |
| `=` | platform (solid from above) |
| `D` | door (solid until opened with the key) |
| `X` | exit |
| `P` | player spawn |
| `A` | aswang |
| `K` | kapre |
| `T` | tikbalang |
| `N` | manananggal |
| `B` | boss |
| `H` | health pickup |
| `M` | ammo pickup |
| `Y` | key |

`tools/genlevel.py` reads the stage files and writes `level.inc`. The rules it checks:
all stages are 120x22, a player spawn and an exit are required, a door needs a key,
at most 8 enemies and 8 pickups per stage, and at most 16 spawns per stage.

**Progression.** The game has five stages (`level_src.txt` is stage 1, `stage2_src.txt`
to `stage5_src.txt`). Each stage has a boss. Stage clear happens when the player reaches
the exit **and every enemy is dead**. Enemies that fall into a pit count as dead. After a
stage clears, the next stage loads. Lives and ammo carry over, HP is refilled to 100, and
the key is taken back.

**Mechanics**

| Thing | Value |
| --- | --- |
| Start | HP 100, 3 lives, 20 ammo |
| Death | lose one life and respawn at the stage spawn. The key is lost and the enemies and pickups reset. A door that was already opened stays open. |
| Game over | when lives reach 0, then R returns to the title |
| Health pickup | +25 HP, capped at 100 |
| Key | opens the door when you face it |
| Aswang | 3 HP, 10 damage on contact |
| Kapre | 6 HP, turret, 15 damage |
| Tikbalang | 4 HP, charges at the player, 15 damage |
| Manananggal | 4 HP, flies, 10 damage |
| Boss | 16 HP, walks at the player and throws aimed fireballs, 20 damage |

**Controls**

| Key | Action |
| --- | --- |
| Arrows | move |
| Space | jump |
| Ctrl | fire |
| Enter | start, or next stage |
| R | restart after game over, or to the title after the final stage |
| Z, X | not used |
| ESC | quit to the OS (also on the title screen) |

---

## Build targets

### `os/Makefile`

| Target | What it does |
| --- | --- |
| `make` / `make all` | builds `os.img`. Also builds `../shooter/game.bin` first. |
| `make run` | starts `os.img` in QEMU with sound |
| `make assets` | regenerates the generated assets from `../assets/`. Needs Python packages, ffmpeg, and the source files. |
| `make clean` | deletes the built files (`boot.bin`, `stage2.bin`, `kernel.bin`, `os.img`) |

### `shooter/Makefile`

| Target | What it does |
| --- | --- |
| `make` / `make all` | builds `shooter.img` (standalone game floppy) |
| `make game.bin` | builds only the game binary, which the OS also uses |
| `make run` | starts `shooter.img` in QEMU with sound |
| `make smoke` | headless QEMU test. Starts the game, takes a screenshot to `images/smoke.ppm`, and checks it has colours. |
| `make extract` | regenerates sprites and the background from `../assets/stitch_pixel_art_asset_extractor/`. Needs numpy, Pillow, scipy. |
| `make clean` | deletes `game.bin`, `boot.bin`, `shooter.img`, and the smoke screenshot |

Build size limit: `game.bin` must stay under 192 KB (`196608` bytes), because the game's
framebuffer starts at `0x40000`. The Makefile checks this.

---

## Known limitations

- The text viewer breaks lines by character count, so a word can be split across two lines.
- Sound (PC speaker) has been checked by code and screenshots only. It has not been heard.
- The door a player opened stays open after a death, even though the key is lost.
- ESC is the only quit key in the game. Q is not used there.
- Pickups and enemies return to their starting positions after a death, but the map keeps the opened doors.
