#!/usr/bin/env python3
"""genlevel.py - convert stage sources into level.inc (NASM)

source chars:
  . empty   # solid ground   = platform   D door (locked)   X exit
  P player spawn   A aswang   K kapre   T tikbalang   N manananggal
  B boss   H health   M ammo   Y key
  lines starting with ';' are comments

usage:
  genlevel.py level.inc stage1.txt [stage2.txt ...]   (first file = stage 1)
  genlevel.py --preview stage.txt
"""
import sys
from collections import deque

TILE = {".": 0, "#": 1, "=": 2, "D": 3, "X": 4}
SPAWN = {"P": 0, "A": 1, "K": 2, "H": 3, "M": 4, "Y": 5,
         "T": 6, "N": 7, "B": 8}
SPAWN_NAMES = {0: "player", 1: "aswang", 2: "kapre", 3: "health",
               4: "ammo", 5: "key", 6: "tikbalang", 7: "manananggal",
               8: "boss"}
ENEMY_TYPES = (1, 2, 6, 7, 8)       # MAX_ENEMIES 8 in enemies.inc
PICKUP_TYPES = (3, 4, 5)            # MAX_PICKUPS 8 in pickups.inc
SOLID = {"#", "=", "D"}
MAX_LEVEL_SPAWNS = 16               # RAM buffer size in level.inc
MAP_W_FIXED = 120                   # all stages share the same map size
MAP_H_FIXED = 22


def fail(msg):
    print(f"genlevel ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def warn(msg):
    print(f"genlevel warn: {msg}", file=sys.stderr)


def load(src):
    rows = []
    for line in open(src).read().splitlines():
        line = line.rstrip("\r")
        if line.startswith(";"):
            continue
        rows.append(line)
    if not rows:
        fail(f"{src}: empty source")

    width = max(len(r) for r in rows)
    height = len(rows)
    if width < 24 or height < 12:
        fail(f"{src}: map too small ({width}x{height}), need >= 24x12")
    if width > 256 or height > 64:
        fail(f"{src}: map too large ({width}x{height}), max 256x64")
    rows = [r.ljust(width, ".") for r in rows]

    valid = set(TILE) | set(SPAWN)
    spawns = {}          # type -> list of (tx, ty)
    player = None
    exit_pos = None
    key_pos = None
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch not in valid:
                fail(f"{src}: unknown char {ch!r} at ({x},{y})")
            if ch in SPAWN:
                t = SPAWN[ch]
                spawns.setdefault(t, []).append((x, y))
                if t == 0:
                    player = (x, y)
                if t == 5:
                    key_pos = (x, y)
            elif ch == "X":
                exit_pos = (x, y)

    if player is None:
        fail(f"{src}: no player spawn 'P'")
    if len(spawns.get(0, [])) > 1:
        fail(f"{src}: more than one player spawn 'P'")
    if exit_pos is None:
        fail(f"{src}: no exit 'X'")
    if sum(len(spawns.get(t, [])) for t in ENEMY_TYPES) > 8:
        fail(f"{src}: more than 8 enemies (MAX_ENEMIES)")
    if sum(len(spawns.get(t, [])) for t in PICKUP_TYPES) > 8:
        fail(f"{src}: more than 8 pickups (MAX_PICKUPS)")
    if sum(len(v) for v in spawns.values()) > MAX_LEVEL_SPAWNS:
        fail(f"{src}: more than {MAX_LEVEL_SPAWNS} spawns")

    has_door = any("D" in r for r in rows)
    has_key = key_pos is not None
    if has_door and not has_key:
        fail(f"{src}: door 'D' present but no key 'Y' - level unbeatable")
    if has_key and not has_door:
        warn(f"{src}: key 'Y' present but no door 'D'")

    # reachability (4-dir walk over non-solid cells; soft check)
    def reachable(target, allow_door=False):
        seen = {(player[0], player[1])}
        q = deque(seen)
        while q:
            x, y = q.popleft()
            if (x, y) == target:
                return True
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if not (0 <= nx < width and 0 <= ny < height) \
                        or (nx, ny) in seen:
                    continue
                ch = rows[ny][nx]
                if ch in SOLID and not (allow_door and ch == "D"):
                    continue
                seen.add((nx, ny))
                q.append((nx, ny))
        return False

    key_ok = True
    if has_key:
        key_ok = reachable(key_pos)
        if not key_ok:
            warn(f"{src}: key {key_pos} not 4-dir reachable from player")
    if not reachable(exit_pos, allow_door=has_key and key_ok):
        warn(f"{src}: exit {exit_pos} not reachable (check gaps/door)")

    return dict(src=src, rows=rows, width=width, height=height,
                spawns=spawns, player=player)


def emit_stage(lines, n, st):
    rows, spawns = st["rows"], st["spawns"]
    width, height = st["width"], st["height"]
    px, py = st["player"]
    entries = []
    for t in (1, 2, 6, 7, 8, 3, 4, 5):
        for (x, y) in spawns.get(t, []):
            entries.append(f"    db {t}, {x * 16 & 0xFF}, {x * 16 >> 8}, "
                           f"{y * 16 & 0xFF}, {y * 16 >> 8}"
                           f"   ; {SPAWN_NAMES[t]} @ tile {x},{y}")
    lines.append(f"; ---- stage {n}: {st['src']} ----")
    lines.append(f"s{n}_player: dw {px * 16}, {py * 16}")
    lines.append(f"s{n}_spawns:")
    lines.append(f"    db {len(entries)}")
    lines.extend(entries if entries else ["    db 0"])
    lines.append(f"s{n}_map:")
    for y, row in enumerate(rows):
        ids = []
        for x, ch in enumerate(row):
            tid = TILE[ch] if ch in TILE else 0
            # topmost '#' of each column gets the lit-surface cap tile
            if tid == 1 and (y == 0 or rows[y - 1][x] != "#"):
                tid = 5
            ids.append(str(tid))
        lines.append("    db " + ",".join(ids))


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1

    if sys.argv[1] == "--preview":
        st = load(sys.argv[2])
        for y, row in enumerate(st["rows"]):
            print(f"{y:3d} {row}")
        print("    " + "".join(str(i // 10 % 10) for i in range(st["width"])))
        print("    " + "".join(str(i % 10) for i in range(st["width"])))
        return 0

    out, srcs = sys.argv[1], sys.argv[2:]
    stages = [load(s) for s in srcs]
    for st in stages:
        if (st["width"], st["height"]) != (MAP_W_FIXED, MAP_H_FIXED):
            fail(f"{st['src']}: size {st['width']}x{st['height']}, "
                 f"all stages must be {MAP_W_FIXED}x{MAP_H_FIXED}")

    lines = []
    lines.append("; AUTO-GENERATED by tools/genlevel.py - DO NOT EDIT")
    lines.append("; sources: " + ", ".join(srcs))
    lines.append(f"MAP_W equ {MAP_W_FIXED}")
    lines.append(f"MAP_H equ {MAP_H_FIXED}")
    lines.append(f"MAX_LEVEL_SPAWNS equ {MAX_LEVEL_SPAWNS}")
    lines.append(f"stage_total: dd {len(stages)}")
    lines.append("; per stage: map ptr, spawn list ptr, player spawn ptr")
    lines.append("stage_table:")
    for n in range(1, len(stages) + 1):
        lines.append(f"    dd s{n}_map, s{n}_spawns, s{n}_player")
    for n, st in enumerate(stages, 1):
        emit_stage(lines, n, st)
    # RAM copy of the active stage, filled by stage_load in game.asm
    lines.append("level_player_spawn: dw 0, 0")
    lines.append("level_spawn_count: db 0")
    lines.append("level_spawns: times MAX_LEVEL_SPAWNS * 5 db 0")
    lines.append("map_data: times MAP_W * MAP_H db 0")
    open(out, "w").write("\n".join(lines) + "\n")
    print(f"genlevel: {len(stages)} stage(s) -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
