#!/usr/bin/env python3
"""extract.py - cut sprites/tiles out of the AI-generated PNG sheets, scale them
to 16x16 (32x32 boss), snap to the game's 64-colour VGA palette and write
tools/sprites_gen.py (ASCII rows) for genart.py.
usage: extract.py [sheet_dir]   (default ../../assets/stitch_pixel_art_asset_extractor)
Also writes images/preview.png (8x upscale contact sheet).
"""
import os, re, sys, glob
import numpy as np
from PIL import Image
from scipy import ndimage as ndi

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    HERE, "..", "..", "assets", "stitch_pixel_art_asset_extractor")

def sheet(key):
    p = glob.glob(os.path.join(SRC, f"*{key}*", "screen.png"))
    if not p:
        sys.exit(f"sheet '{key}' not found in {SRC}")
    return Image.open(p[0]).convert("RGB")

def load_palette():
    txt = open(os.path.join(HERE, "..", "video.inc")).read()
    txt = txt.split("palette_data:")[1].split("; ----")[0]
    nums = []
    for line in txt.splitlines():
        line = line.split(";")[0].strip()
        if line.startswith("db"):
            nums += [int(x) for x in line[2:].replace(" ", "").split(",") if x]
    assert len(nums) == 192, len(nums)
    return np.array(nums, float).reshape(64, 3) * 255 / 63

PAL = load_palette()
# chars NOT used by hand-written charset_pairs / quote / '.'
_BAD = set('k w s n b r g y p e f d c o m T U V W Z I 0 1 2 3 4 5 6 7 8 9 " . #'.split()) | {'"', '.', '#', ' '}
ALPHA = [chr(c) for c in range(33, 127) if chr(c) not in _BAD]
class _CH(dict):
    def __missing__(self, k):
        self[k] = ALPHA[len(self)]
        return self[k]
CH = _CH()

def snap(rgb):
    d = ((rgb[:, None, :] - PAL[None, 1:, :]) ** 2 * np.array([.30, .59, .11])).sum(2)
    return d.argmin(1) + 1

def cut(img, box, size, keepbg=False, bottom=True, bgthr=60, gain=1.0):
    c = img.crop(box)
    a = np.asarray(c).astype(int)
    if keepbg:
        im = c.resize((size, size), Image.BOX)
        idx = snap(np.asarray(im).reshape(-1, 3).astype(float)).reshape(size, size)
        return idx
    # background = border-median colour; flood from edges
    border = np.concatenate([a[0], a[-1], a[:, 0], a[:, -1]])
    bg = np.median(border, axis=0)
    near = np.abs(a - bg).sum(2) < bgthr
    lab, n = ndi.label(near)
    edge = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]])))
    edge.discard(0)
    bgmask = np.isin(lab, list(edge))
    fg = ~bgmask
    fg = ndi.binary_opening(fg, iterations=1)
    lab, n = ndi.label(fg)
    if n > 1:                                   # keep big blobs only
        sizes = ndi.sum(fg, lab, range(1, n + 1))
        keep = [i + 1 for i, s in enumerate(sizes) if s > fg.sum() * 0.02]
        fg = np.isin(lab, keep)
    ys, xs = np.where(fg)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    rgba = np.dstack([a, fg * 255]).astype(np.uint8)[y0:y1, x0:x1]
    h, w = rgba.shape[:2]
    s = min(size / w, size / h)
    nw, nh = max(1, round(w * s)), max(1, round(h * s))
    # premultiplied downscale
    pm = rgba.astype(float)
    pm[..., :3] *= pm[..., 3:] / 255
    big = Image.fromarray(pm.astype(np.uint8), "RGBA")
    parts = [np.asarray(big.resize((nw, nh), Image.BOX).getchannel(k)).astype(float) for k in range(4)]
    al = parts[3] / 255
    rgb = np.dstack(parts[:3]) / np.maximum(al[..., None], 1e-3)
    out = np.zeros((size, size), int)
    ox = (size - nw) // 2
    oy = (size - nh) if bottom else (size - nh) // 2
    rgb = np.clip(rgb * gain, 0, 255)
    idx = snap(rgb.reshape(-1, 3)).reshape(nh, nw)
    idx[al < 0.45] = 0
    out[oy:oy + nh, ox:ox + nw] = idx
    return out

def rows(idx):
    return ["".join("." if v == 0 else CH[v] for v in r) for r in idx]

H = sheet("protagonist"); T = sheet("tileset"); I = sheet("items_collect")
AS = sheet("aswang"); TK = sheet("tikbalang"); KP = sheet("kapre")
BG = sheet("background")

SPR, SPR32, TIL = {}, {}, {}
# --- player
for name, box in {
    "spr_player_idle":  (45, 38, 135, 192),
    "spr_player_run1":  (35, 215, 155, 358),
    "spr_player_run2":  (180, 215, 300, 358),
    "spr_player_jump":  (510, 638, 620, 815),
    "spr_player_shoot": (35, 858, 150, 1022),
}.items():
    SPR[name] = cut(H, box, 16)
# --- enemies
SPR["spr_aswang1"] = cut(AS, (48, 65, 285, 344), 16)
SPR["spr_aswang2"] = cut(AS, (329, 86, 664, 334), 16)
SPR["spr_tikbalang1"] = cut(TK, (20, 50, 226, 324), 16, bgthr=18, gain=1.5)
SPR["spr_tikbalang2"] = cut(TK, (230, 50, 437, 324), 16, bgthr=18, gain=1.5)
SPR["spr_kapre1"] = cut(KP, (64, 22, 526, 505), 16, bgthr=18, gain=2.2)
SPR["spr_kapre2"] = cut(KP, (571, 20, 984, 505), 16, bgthr=18, gain=2.2)
SPR32["spr_boss1"] = cut(KP, (38, 524, 490, 990), 32, bgthr=18, gain=2.2)
SPR32["spr_boss2"] = cut(KP, (570, 521, 999, 990), 32, bgthr=18, gain=2.2)
# --- pickups
SPR["spr_key"] = cut(I, (270, 50, 372, 122), 16, bottom=False)
SPR["spr_health"] = cut(I, (622, 35, 692, 125), 16, bottom=False)
SPR["spr_ammo"] = cut(I, (135, 372, 252, 460), 16, bottom=False)
# --- tiles (stretch, no bg removal) ---
STONE_BOX = (768, 768, 896, 896)            # blue-gray rubble rock
TIL["tile_art_ground"] = cut(T, STONE_BOX, 16, keepbg=True)
# lit surface cap for the topmost ground tile (brightened top 2 rows)
top = np.asarray(T.crop(STONE_BOX).resize((16, 16), Image.BOX)).astype(float)
top[:2] = np.clip(top[:2] * 1.55, 0, 255)
TIL["tile_art_ground_top"] = snap(top.reshape(-1, 3)).reshape(16, 16)
# platform: steel I-beam strip in the top 8 rows (rest transparent)
beam = np.asarray(T.crop((0, 545, 384, 600)).resize((16, 8), Image.BOX)).astype(float)
plat = np.zeros((16, 16), int)
plat[:8] = snap(beam.reshape(-1, 3)).reshape(8, 16)
TIL["tile_art_platform"] = plat
TIL["tile_art_door"] = cut(I, (520, 515, 745, 765), 16, keepbg=True)
TIL["tile_art_exit"] = cut(I, (770, 515, 1000, 765), 16, keepbg=True)

# --- scrolling backdrop: illustrated cave/industrial scene -> raw palette
# indices. 4:3 crop (skips SECTOR labels) scaled to 320x240, mirrored to
# 640x240 so the horizontal wrap seam is invisible; gain 0.75 keeps it dark
# so foreground pops. Blitted by video.inc:draw_backdrop (cam_x/4 parallax).
BG_BOX, BG_GAIN = (0, 36, 976, 768), 0.75
bg = BG.crop(BG_BOX).resize((320, 240), Image.BOX)
bgi = snap(np.asarray(bg).reshape(-1, 3).astype(float) * BG_GAIN).reshape(240, 320)
bgi = np.concatenate([bgi, bgi[:, ::-1]], axis=1)          # seamless wrap
assert bgi.shape == (240, 640)
open(os.path.join(HERE, "bg.bin"), "wb").write(bgi.astype(np.uint8).tobytes())
print("wrote bg.bin: 640x240 palette indices (mirrored, seamless)")

used = set()
out = ["# AUTO-GENERATED by tools/extract.py - DO NOT EDIT"]
for dname, d in (("SPRITES", SPR), ("SPRITES32", SPR32), ("TILES", TIL)):
    out.append(f"{dname} = {{")
    for k, v in d.items():
        used |= set(np.unique(v)) - {0}
        out.append(f"    {k!r}: {rows(v)!r},")
    out.append("}")
out.append("PAIRS = " + repr({CH[int(i)]: int(i) for i in sorted(used)}))
open(os.path.join(HERE, "sprites_gen.py"), "w").write("\n".join(out) + "\n")

# preview
def render(idx, s=8):
    im = np.zeros(idx.shape + (3,), np.uint8) + 40
    m = idx > 0
    im[m] = PAL[idx[m]].astype(np.uint8)
    return Image.fromarray(im).resize((idx.shape[1] * s, idx.shape[0] * s), Image.NEAREST)
allv = list(SPR.values()) + list(TIL.values()) + list(SPR32.values())
W = 8 * 33 * 6
sheetim = Image.new("RGB", (W, 8 * 34 * 4), (40, 40, 40))
x = y = 0
for v in allv:
    r = render(v)
    if x + r.width > W: x = 0; y += 8 * 34
    sheetim.paste(r, (x, y)); x += r.width + 8
sheetim.save(os.path.join(HERE, "..", "images", "preview.png"))
print("wrote sprites_gen.py, preview.png;", len(used), "palette colours used")
