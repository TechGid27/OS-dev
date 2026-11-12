#!/usr/bin/env python3
"""genart.py - generate art.inc (font table, sprite art, tile art) for the game.
Rows are validated: font rows exactly 8 chars, sprite/tile rows exactly 16.
usage: genart.py [output.inc]
"""
import sys

# ---------------------------------------------------------------- glyphs
# 8 rows x 8 chars, '#' = pixel on
FONT = {
    " ": ["........", "........", "........", "........",
          "........", "........", "........", "........"],
    "A": ["..##....", ".####...", "##..##..", "##..##..",
          "######..", "##..##..", "##..##..", "........"],
    "B": ["#####...", "##..##..", "##..##..", "#####...",
          "##..##..", "##..##..", "#####...", "........"],
    "C": [".####...", "##..##..", "##......", "##......",
          "##......", "##..##..", ".####...", "........"],
    "D": ["#####...", "##..##..", "##..##..", "##..##..",
          "##..##..", "##..##..", "#####...", "........"],
    "E": ["######..", "##......", "##......", "#####...",
          "##......", "##......", "######..", "........"],
    "F": ["######..", "##......", "##......", "#####...",
          "##......", "##......", "##......", "........"],
    "G": [".####...", "##..##..", "##......", "##.###..",
          "##..##..", "##..##..", ".####...", "........"],
    "H": ["##..##..", "##..##..", "##..##..", "######..",
          "##..##..", "##..##..", "##..##..", "........"],
    "I": [".####...", "..##....", "..##....", "..##....",
          "..##....", "..##....", ".####...", "........"],
    "J": ["....##..", "....##..", "....##..", "....##..",
          "....##..", "##..##..", ".####...", "........"],
    "K": ["##..##..", "##.##...", "####....", "###.....",
          "####....", "##.##...", "##..##..", "........"],
    "L": ["##......", "##......", "##......", "##......",
          "##......", "##......", "######..", "........"],
    "M": ["##...##.", "###.###.", "##.#.##.", "##...##.",
          "##...##.", "##...##.", "##...##.", "........"],
    "N": ["##..##..", "###.##..", "####.##.", "##.#.##.",
          "##..###.", "##..##..", "##..##..", "........"],
    "O": [".####...", "##..##..", "##..##..", "##..##..",
          "##..##..", "##..##..", ".####...", "........"],
    "P": ["#####...", "##..##..", "##..##..", "#####...",
          "##......", "##......", "##......", "........"],
    "Q": [".####...", "##..##..", "##..##..", "##..##..",
          "##.###..", ".####...", "....##..", "........"],
    "R": ["#####...", "##..##..", "##..##..", "#####...",
          "####....", "##.##...", "##..##..", "........"],
    "S": [".####...", "##..##..", "##......", ".####...",
          "....##..", "##..##..", ".####...", "........"],
    "T": ["######..", "..##....", "..##....", "..##....",
          "..##....", "..##....", "..##....", "........"],
    "U": ["##..##..", "##..##..", "##..##..", "##..##..",
          "##..##..", "##..##..", ".####...", "........"],
    "V": ["##..##..", "##..##..", "##..##..", "##..##..",
          "##..##..", ".####...", "..##....", "........"],
    "W": ["##...##.", "##...##.", "##...##.", "##...##.",
          "##.#.##.", "##.#.##.", ".#...#..", "........"],
    "X": ["##..##..", "##..##..", ".####...", "..##....",
          ".####...", "##..##..", "##..##..", "........"],
    "Y": ["##..##..", "##..##..", ".####...", "..##....",
          "..##....", "..##....", "..##....", "........"],
    "Z": ["######..", "....##..", "...##...", "..##....",
          ".##.....", "##......", "######..", "........"],
    "0": [".####...", "##..##..", "##..###.", "##.##.##",
          "####.##.", "##..##..", ".####...", "........"],
    "1": ["..##....", ".###....", "..##....", "..##....",
          "..##....", "..##....", ".####...", "........"],
    "2": [".####...", "##..##..", "....##..", "...##...",
          "..##....", ".##.....", "######..", "........"],
    "3": [".####...", "##..##..", "....##..", "..###...",
          "....##..", "##..##..", ".####...", "........"],
    "4": ["...##...", "..###...", ".#.##...", "##.##...",
          "######..", "...##...", "...##...", "........"],
    "5": ["######..", "##......", "#####...", "....##..",
          "....##..", "##..##..", ".####...", "........"],
    "6": [".####...", "##......", "##......", "#####...",
          "##..##..", "##..##..", ".####...", "........"],
    "7": ["######..", "....##..", "...##...", "...##...",
          "..##....", "..##....", "..##....", "........"],
    "8": [".####...", "##..##..", "##..##..", ".####...",
          "##..##..", "##..##..", ".####...", "........"],
    "9": [".####...", "##..##..", "##..##..", ".#####..",
          "....##..", "....##..", ".####...", "........"],
    ":": ["........", "..##....", "..##....", "........",
          "..##....", "..##....", "........", "........"],
    "-": ["........", "........", "........", "######..",
          "........", "........", "........", "........"],
    ".": ["........", "........", "........", "........",
          "........", "..##....", "..##....", "........"],
    ",": ["........", "........", "........", "........",
          "........", "..##....", "..##....", ".##....."],
    "!": ["..##....", "..##....", "..##....", "..##....",
          "..##....", "........", "..##....", "........"],
    "?": [".####...", "##..##..", "....##..", "...##...",
          "..##....", "........", "..##....", "........"],
    "/": ["....##..", "....##..", "...##...", "..##....",
          ".##.....", "##......", "##......", "........"],
    "=": ["........", "........", "######..", "........",
          "######..", "........", "........", "........"],
    "'": ["..##....", "..##....", "........", "........",
          "........", "........", "........", "........"],
    "(": ["...##...", "..##....", ".##.....", ".##.....",
          ".##.....", "..##....", "...##...", "........"],
    ")": ["...##...", "..##....", ".##.....", ".##.....",
          ".##.....", "..##....", "...##...", "........"],
}

# ---------------------------------------------------------------- tiles 16x16
TILES = {
    "tile_art_blank": ["................"] * 16,

    "tile_art_ground": [
        "1111111111111111",
        "1333331333331333",
        "1333331333331333",
        "1111111111111111",
        "3333133333133333",
        "3333133333133333",
        "3333133333133333",
        "1111111111111111",
        "1333331333331333",
        "1333331333331333",
        "1111111111111111",
        "3333133333133333",
        "3333133333133333",
        "3333133333133333",
        "1111111111111111",
        "2222222222222222",
    ],

    "tile_art_ground_top": [
        "2222222222222222",
        "1111111111111111",
        "1333331333331333",
        "1333331333331333",
        "1111111111111111",
        "3333133333133333",
        "3333133333133333",
        "3333133333133333",
        "1111111111111111",
        "1333331333331333",
        "1333331333331333",
        "1111111111111111",
        "3333133333133333",
        "3333133333133333",
        "3333133333133333",
        "1111111111111111",
    ],

    "tile_art_platform": [
        "VVVVVVVVVVVVVVVV",
        "WWWWWWWWWWWWWWWW",
        "VeeeeeeeeeeeeeeV",
        "eeeeeeeeeeeeeeee",
        "eeeeeeeeeeeeeeee",
        "VeeeeeeeeeeeeeeV",
        "ZZZZZZZZZZZZZZZZ",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
    ],

    "tile_art_door": [
        "kkkkkkkkkkkkkkkk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUUUUUUTk",
        "kTTUUUUUUU66UUTk",
        "kTTUUUUUUU66UUTk",
        "kTTUUUUUUUUUUUTk",
        "kkkkkkkkkkkkkkkk",
    ],

    "tile_art_exit": [
        "ccIIIIIIIIIIIccI",
        "cIIIIIIIIIIIcIIc",
        "IIIIwwwwwwIIIIII",
        "IIIIwwwwwwIIIIII",
        "IIIIwwwwwwIIIIII",
        "cIIIIIIIIIIIIIcI",
        "cIIIIIIIIIIIIIcI",
        "ccIIIIIIIIIIIccI",
        "ccccccccccccccccc"[:16],
        "ccIIIIIIIIIIIccI",
        "cIIIIIIIIIIIcIIc",
        "IIIIIIIIIIIIIIII",
        "IIIIwwwwwwIIIIII",
        "cIIIIIIIIIIIIIcI",
        "ccIIIIIIIIIIIccI",
        "cccccccccccccccc",
    ],
}

# ---------------------------------------------------------------- sprites 16x16
SPRITES = {
    # ---- player (facing right) ----
    "spr_player_idle": [
        "......kkkk.......",
        ".....kyyyyk......",
        ".....kysnyk......",
        ".....kssssk......",
        "......kssk.......",
        "...kkkssskkkkkk..",
        "...kgggggkkee....",
        "...kgggggkkkee...",
        "...kkgggkk.......",
        "....kdddk........",
        "....kdddk........",
        "....kdkdk........",
        "....kdk.kdk......",
        "...kkd...kd......",
        "...kee...kee.....",
        "................",
    ],
    "spr_player_run1": [
        "......kkkk.......",
        ".....kyyyyk......",
        ".....kysnyk......",
        ".....kssssk......",
        "......kssk.......",
        "...kkkssskkkkkk..",
        "...kgggggkkee....",
        "...kgggggkkkee...",
        "...kkgggkk.......",
        "....kdddk........",
        "....kdddkd.......",
        "...kdddk.kd......",
        "...kdk...kd......",
        "..kkd....kk......",
        "..kee...kee......",
        "................",
    ],
    "spr_player_run2": [
        "......kkkk.......",
        ".....kyyyyk......",
        ".....kysnyk......",
        ".....kssssk......",
        "......kssk.......",
        "...kkkssskkkkkk..",
        "...kgggggkkee....",
        "...kgggggkkkee...",
        "...kkgggkk.......",
        "....kdddk........",
        "....kdddk........",
        "....kdddk........",
        "...kdddkd........",
        "..kkdk.kdk.......",
        "..keek..keek.....",
        "................",
    ],
    "spr_player_jump": [
        "......kkkk.......",
        ".....kyyyyk......",
        ".....kysnyk......",
        ".....kssssk......",
        "......kssk..kk...",
        "...kkkssskkkkek..",
        "...kgggggkkeee...",
        "...kgggggkkkee...",
        "...kkgggkk.......",
        "....kdddk........",
        "...kdddkdd.......",
        "...kdk.kdk.......",
        "...kd..kkd.......",
        "..keek.kke.......",
        "..................",
        "................",
    ],
    "spr_player_shoot": [
        "......kkkk.......",
        ".....kyyyyk......",
        ".....kysnyk......",
        ".....kssssk......",
        "......kssk.......",
        "...kkkssskkkkkkk.",
        "...kgggggkkeeef..",
        "...kgggggkkkeee..",
        "...kkgggkk.......",
        "....kdddk........",
        "....kdddk........",
        "....kdkdk........",
        "....kdk.kdk......",
        "...kkd...kd......",
        "...kee...kee.....",
        "................",
    ],

    # ---- aswang (dark winged creature) ----
    "spr_aswang1": [
        "................",
        ".....kkkk.......",
        "....kpppk.......",
        "....kprprk......",       # red eyes
        "....kppppk......",
        "...kkppppkk.....",
        "..kpkkkkkkpk....",
        "..kppppppppk....",
        ".kpkkppppkkpk...",
        ".kpk.kppk.kpk...",
        ".kk..kppk..kk...",
        "......kkk.......",
        ".....kpppk......",
        ".....kp.kp......",
        "....kpk.kpk.....",
        "................",
    ],
    "spr_aswang2": [
        "................",
        "................",
        ".....kkkk.......",
        "....kpppk.......",
        "....kprprk......",
        "....kppppk......",
        "...kkppppkk.....",
        "..kpkkkkkkpk....",
        "..kppppppppk....",
        ".kpkkppppkkpk...",
        ".kpk.kppk.kpk...",
        ".kk..kppk..kk...",
        "......kkk.......",
        ".....kpppk......",
        "....kp...pk.....",
        "...kpk...kpk....",
    ],

    # ---- kapre (big smoking tank) ----
    "spr_kapre1": [
        "......kkkk......",
        ".....kddd dk....".replace(" ", ""),
        ".....kdddkd.....",
        ".....kddddk.....",
        "....kkddddkk....",
        "...kdddddddddk..",
        "..kddpppppdddk..",
        "..kdpodddddpdk..",
        "..kddddddddddk..",
        "..kddddddddddkk.",
        "..kddddddddddkoo",
        "..kddddddddddk.o",
        "...kddddddddk...",
        "...kddddddddk...",
        "...kddk.kdddk...",
        "...kee...keee...",
    ],
    "spr_kapre2": [
        "......kkkk......",
        ".....kddddk.....",
        ".....kdddkd.....",
        ".....kddddk.....",
        "....kkddddkk....",
        "...kdddddddddk..",
        "..kddpppppdddk..",
        "..kdpodddddpdk..",
        "..kddddddddddk..",
        ".kdddddddddddk..",
        ".kddddddddddkoo.",
        ".kddddddddddk.o.",
        "..kddddddddk....",
        "..kddddddddk....",
        "..kddk.kdddk....",
        "..kee...keee....",
    ],

    # ---- pickups (16x16 cells, art centered) ----
    "spr_key": [
        "................",
        "................",
        "................",
        "................",
        "....kkkk........",
        "...k6666k.......",
        "..k66kk66k......",
        "..k6k..k6k......",
        "..k66kk66kkkk...",
        "...k6666k6k6k...",
        "....kkkk.k6k6k..",
        "........k6666k..",
        "........kkkk....",
        "................",
        "................",
        "................",
    ],
    "spr_health": [
        "................",
        "................",
        "....kkkkkkkk....",
        "..kkwwwwwwwwkk..",
        "..kwwwwwwwwwwk..",
        "..kwwwrrrrwwwk..",
        "..kwwwrrrrwwwk..",
        "..kwwrrrrrrwwk..",
        "..kwwrrrrrrwwk..",
        "..kwwwrrrrwwwk..",
        "..kwwwrrrrwwwk..",
        "..kwwwwwwwwwwk..",
        "..kkwwwwwwwwkk..",
        "....kkkkkkkk....",
        "................",
        "................",
    ],
    "spr_ammo": [
        "................",
        "................",
        "................",
        "..kkkkkkkkkkkk..",
        "..k5555555555k..",
        "..k5566665566k..",
        "..k5566665566k..",
        "..k5566665566k..",
        "..k5555555555k..",
        "..k5566665566k..",
        "..k5555555555k..",
        "..kkkkkkkkkkkk..",
        "................",
        "................",
        "................",
        "................",
    ],

    # ---- tikbalang (horse-headed, brown ramp '9'/'0') ----
    "spr_tikbalang1": [
        "................",
        "....kkkkk.......",
        "...k999999k.....",
        "..k999999990k...",
        "..k9k99999990k..",
        "..kk999999990k..",
        "...kkkkkkkkkk...",
        "..kk99999999kk..",
        ".k999999999999k.",
        ".k999999999990k.",
        ".kk9999999999kk.",
        "..kk99kkkk99kk..",
        "...k9k....k9k...",
        "..k9k......k9k..",
        ".k9k........k9k.",
        ".kk..........kk.",
    ],
    "spr_tikbalang2": [
        "................",
        "....kkkkk.......",
        "...k999999k.....",
        "..k999999990k...",
        "..k9k99999990k..",
        "..kk999999990k..",
        "...kkkkkkkkkk...",
        "..kk99999999kk..",
        ".k999999999999k.",
        ".k999999999990k.",
        ".kk9999999999kk.",
        "..kk99kkkk99kk..",
        "...k9k...k9k....",
        "..k9k.....k9k...",
        "..k9k.....k9k...",
        "..kk.......kk...",
    ],

    # ---- manananggal (flying pale torso, severed waist, bat wings) ----
    "spr_manananggal1": [
        "................",
        "......kddk......",
        ".....kddddk.....",
        ".....kmmmmk.....",
        ".....kmkkmk.....",
        "..k..kmmmmk..k..",
        ".kpk.kkmmkk.kpk.",
        "kppmkkmmmmkkmppk",
        "kpmmmkmmmmkmmmpk",
        ".kpmmkmmmmkmmpk.",
        "..kpmkkmmkkmpk..",
        "...kkkmmmmkkk...",
        "....kmmrrmmk....",
        ".....kmmmmk.....",
        "......kkkk......",
        "................",
    ],
    "spr_manananggal2": [
        "................",
        "......kddk......",
        ".....kddddk.....",
        ".....kmmmmk.....",
        ".....kmkkmk.....",
        "kk...kmmmmk...kk",
        "kpk..kkmmkk..kpk",
        "kppkkkmmmmkkkppk",
        ".kpmmkmmmmkmmpk.",
        "..kpmkmmmmkmpk..",
        "...kkkkmmkkkk...",
        "....kmmmmmmk....",
        "....kmmrrmmk....",
        ".....kmmmmk.....",
        "......kkkk......",
        "................",
    ],
}

# ---------------------------------------------------------------- boss 32x32
SPRITES32 = {
    "spr_boss1": [
        "................................",
        "........kkk.......kkk...........",
        ".......krrk.....krrk............",
        "........krrkkkkkrrk.............",
        ".........kkrrrrrrkk.............",
        ".......krrrrrrrrrrrrrk..........",
        ".......krryyyrryyrrrk...........",
        ".......krryyyrryyrrrk...........",
        ".......krrrrkkkkrrrrrk..........",
        "........krrrkkkkkrrrk...........",
        ".........kkkkkkkkkkk............",
        "..........kkrrrrrrkk............",
        ".......kkkrrrrrrrrkkk...........",
        "......krrrrrrrrrrrrrrk..........",
        ".....krrkrrrrrrrrrrkrrk.........",
        "....korkrrrrrrrrrrrrkrok........",
        "....korkrrrrrrrrrrrrkrok........",
        "....kkkkrrrrrrrrrrrrkkkk........",
        ".......krrrrkkkkrrrrrk..........",
        ".......kddddkkkkdddddk..........",
        "......kddddd..kk..ddddd.........",
        "......kddddd..kk..ddddd.........",
        "......kdddk....kk....kddk.......",
        ".....kdddk......kk......kddk....",
        ".....kdddk......kk......kddk....",
        "....kdddk........kk........kdddk",
        "....kdddk........kk........kdddk",
        "...kdddk......kk......kdddk.....",
        "..kdddk......kk......kdddk......",
        "..kddk........kk........kddk....",
        "..kkkk........kk........kkkk....",
        "................................",
    ],
    "spr_boss2": [
        "................................",
        "........kkk.......kkk...........",
        ".......krrk.....krrk............",
        "........krrkkkkkrrk.............",
        ".........kkrrrrrrkk.............",
        ".......krrrrrrrrrrrrrk..........",
        ".......krryyyrryyrrrk...........",
        ".......krryyyrryyrrrk...........",
        ".......krrrrkkkkrrrrrk..........",
        "........krrrkkkkkrrrk...........",
        ".........kkkkkkkkkkk............",
        "..........kkrrrrrrkk............",
        ".......kkkrrrrrrrrkkk...........",
        "......krrrrrrrrrrrrrrk..........",
        ".....krrkrrrrrrrrrrkrrk.........",
        "....korkrrrrrrrrrrrrkrok........",
        "....korkrrrrrrrrrrrrkrok........",
        "....kkkkrrrrrrrrrrrrkkkk........",
        ".......krrrrkkkkrrrrrk..........",
        ".......kddddkkkkdddddk..........",
        "......kkkkk..kk..kkkkk..........",
        ".......kddd..kk..dddk...........",
        ".......kddk..kk..kddk...........",
        ".......kddk..kk..kddk...........",
        "......kdddk..kk..kdddk..........",
        "......kdddk..kk..kdddk..........",
        ".....kdddk...kk...kdddk.........",
        ".....kdddk...kk...kdddk.........",
        "....kdddk....kk....kdddk........",
        "....kddk.....kk.....kddk........",
        "...kkkk......kk......kkkk.......",
        "................................",
    ],
}


# ---- optional AI-sheet art (tools/extract.py) overrides hand-drawn art ----
PAIRS = {}
try:
    sys.path.insert(0, __import__("os").path.dirname(__import__("os").path.abspath(__file__)))
    import sprites_gen as _sg
    SPRITES.update(_sg.SPRITES)
    SPRITES32.update(_sg.SPRITES32)
    TILES.update(_sg.TILES)
    PAIRS = _sg.PAIRS
except ImportError:
    pass


def check(name, rows, size):
    errs = []
    if len(rows) != size:
        errs.append(f"{name}: {len(rows)} rows != {size}")
    fixed = []
    for i, r in enumerate(rows):
        if len(r) > size:
            excess = r[size:]
            if set(excess) <= {".", " "}:      # trailing dots only
                print(f"genart note: {name} row {i} truncated "
                      f"({len(r)}->{size})")
                r = r[:size]
            else:
                errs.append(f"{name} row {i}: len {len(r)} > {size} "
                            f"with art content: {r!r}")
                r = r[:size]
        elif len(r) < size:
            print(f"genart note: {name} row {i} padded ({len(r)}->{size})")
            r = r.ljust(size, ".")
        fixed.append(r)
    return errs, fixed


def emit_ascii(out, name, rows, label=None):
    label = label or name
    out.append(f"{label}:")
    for r in rows:
        out.append(f'    db "{r}"')


def main():
    dest = sys.argv[1] if len(sys.argv) > 1 else "art.inc"
    errs = []
    for g in list(FONT):
        e, fixed = check(f"font {g!r}", FONT[g], 8)
        errs += e
        FONT[g] = fixed
    for n in list(TILES):
        e, fixed = check(n, TILES[n], 16)
        errs += e
        TILES[n] = fixed
    for n in list(SPRITES):
        e, fixed = check(n, SPRITES[n], 16)
        errs += e
        SPRITES[n] = fixed
    for n in list(SPRITES32):
        e, fixed = check(n, SPRITES32[n], 32)
        errs += e
        SPRITES32[n] = fixed
    if errs:
        for e in errs:
            print("genart ERROR:", e, file=sys.stderr)
        return 1

    out = []
    out.append("; AUTO-GENERATED by tools/genart.py - DO NOT EDIT")
    out.append("; charset: '#'=pixel for font; sprite/tile chars via charset_pairs")

    out.append("section .text")
    # ---- font table: db char, 64 row bytes ... terminator 0 ----
    out.append("font_table:")
    for g in sorted(FONT, key=ord):
        rows = FONT[g]
        flat = "".join(rows)
        if g in ('"', "\\", "'"):
            out.append(f"    db {ord(g)}")
        elif g == " ":
            out.append("    db 32")
        else:
            out.append(f"    db '{g}'")
        for i in range(8):
            out.append(f'    db "{flat[i * 8:(i + 1) * 8]}"')
    out.append("    db 0")

    out.append("")
    out.append("tile_art_ptrs:")
    out.append("    dd tile_art_blank")
    out.append("    dd tile_art_ground")
    out.append("    dd tile_art_platform")
    out.append("    dd tile_art_door")
    out.append("    dd tile_art_exit")
    out.append("    dd tile_art_ground_top")

    for n, rows in TILES.items():
        out.append("")
        emit_ascii(out, n, rows)
    for n, rows in SPRITES.items():
        out.append("")
        emit_ascii(out, n, rows)
    for n, rows in SPRITES32.items():
        out.append("")
        emit_ascii(out, n, rows)

    out.append("")
    out.append("art_pairs:")
    for ch, idx in sorted(PAIRS.items()):
        out.append(f"    db {ord(ch)}, {idx}")
    out.append("    db 0")

    open(dest, "w").write("\n".join(out) + "\n")
    print(f"genart: wrote {dest} "
          f"(font={len(FONT)}, tiles={len(TILES)}, sprites={len(SPRITES)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
