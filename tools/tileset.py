"""Tilesets: PNG art for painting levels, converted to Game Boy shades.

A tileset is a PNG of 8x8 tiles plus a settings file next to it,
tilesets/<name>.json:

    {
      "image": "dungeon.png",
      "license": "CC0",
      "thresholds": [0.72, 0.5, 0.3],   brightness cut-offs between the 4
                                        shades (lightest first); transparent
                                        pixels are the lightest shade
      "behaviour": {"36": "D", ...}     default map character for a tile:
                                        painting with it sets the cell's
                                        behaviour too (wall, door, ...).
                                        Tiles without one only change the look.
    }

Shade 0 is the lightest, 3 the darkest (as in src/tiles.c).

Cones and aiming must show on painted tiles too, so each painted tile can
need extra copies (see variants()): the Game Boy builds them when a map
loads, from the same rules as art_variant() in src/art.c.
"""
import json
from pathlib import Path

from png import read_png, write_png

ROOT = Path(__file__).resolve().parent.parent
TILESET_DIR = ROOT / "tilesets"
DEFAULT = TILESET_DIR / "dungeon.json"

# Variants, in the order their VRAM slots follow the base tile.
BASE, SEEN, WATCH, AIM, AIM_SEEN = range(5)

# Same dot pattern as TILE_WATCH in tools/gen_tiles.py: where a blind
# guard's watch area lightens a tile.
WATCH_DOTS = [0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00, 0x11]
AIM_ROWS, AIM_MASK = (3, 4), 0x18     # the 2x2 dot in the middle


def brightness(r, g, b):
    return (0.299 * r + 0.587 * g + 0.114 * b) / 255


def load(path=DEFAULT):
    """The tileset's settings plus "tiles": a list of 64 shades per tile."""
    path = Path(path)
    cfg = json.loads(path.read_text())
    w, h, px = read_png(path.parent / cfg["image"])
    t0, t1, t2 = cfg.get("thresholds", [0.72, 0.5, 0.3])

    def shade(p):
        r, g, b, a = p
        if a < 128:
            return 0
        y = brightness(r, g, b)
        return 0 if y > t0 else 1 if y > t1 else 2 if y > t2 else 3

    cols, rows = w // 8, h // 8
    tiles = []
    for ty in range(rows):
        for tx in range(cols):
            tiles.append([shade(px[ty * 8 + y][tx * 8 + x]) for y in range(8) for x in range(8)])
    cfg["tiles"] = tiles
    cfg["columns"] = cols
    cfg["behaviour"] = {int(k): v for k, v in cfg.get("behaviour", {}).items()}
    cfg["path"] = str(path)
    return cfg


def save_taught(taught, path=DEFAULT):
    """Write the shapes auto-tile was taught (autopaint.taught)."""
    path = Path(path)
    cfg = json.loads(path.read_text())
    cfg.setdefault("autopaint", {})["taught"] = dict(sorted(taught.items()))
    path.write_text(json.dumps(cfg, indent=2) + "\n")


def save_behaviour(behaviour, path=DEFAULT):
    """Write the per-tile behaviour back into the settings file."""
    path = Path(path)
    cfg = json.loads(path.read_text())
    cfg["behaviour"] = {str(k): v for k, v in sorted(behaviour.items())}
    path.write_text(json.dumps(cfg, indent=2) + "\n")


def variant(px, kind):
    """Shades of one variant of a tile (64 values). Must match art_variant()."""
    out = list(px)
    for y in range(8):
        for x in range(8):
            i = y * 8 + x
            bit = 0x80 >> x
            if kind in (SEEN, AIM_SEEN) and out[i] == 0:
                out[i] = 1                                   # cone: lighten
            if kind == WATCH and out[i] == 0 and WATCH_DOTS[y] & bit:
                out[i] = 1                                   # watch: dots
            if kind in (AIM, AIM_SEEN) and y in AIM_ROWS and AIM_MASK & bit:
                out[i] = 2 if kind == AIM else 3             # target dot
    return out


def to_2bpp(px):
    """16 bytes of Game Boy tile data."""
    out = []
    for y in range(8):
        lo = hi = 0
        for x in range(8):
            v = px[y * 8 + x]
            lo |= (v & 1) << (7 - x)
            hi |= ((v >> 1) & 1) << (7 - x)
        out += [lo, hi]
    return out


# Which variants a painted tile needs depends on the cells it's painted on:
# cones show on cells the player can stand on and guards can see into
# (floor, exit, mesh); the aiming dots only on floor. Must match draw_tiles()
# in src/main.c.
CONE_CHARS = set(".PEM")
AIM_CHARS = set(".P")


def slots_for(chars):
    """VRAM slots a tile needs, given the map characters it's painted on:
    1 (base), 3 (+ seen, watch) or 5 (+ aim, aim+seen)."""
    if chars & AIM_CHARS:
        return 5
    if chars & CONE_CHARS:
        return 3
    return 1


def parse_art(rows):
    """Art rows ("0a1f..", two hex digits per cell, ".." = none) -> lists."""
    return [[None if row[i:i + 2] == ".." else int(row[i:i + 2], 16)
             for i in range(0, len(row), 2)] for row in rows]


def format_art(grid):
    return ["".join(".." if v is None else f"{v:02x}" for v in row) for row in grid]


def rle_rows(grid):
    """Run-length encode each row as (run, value) byte pairs; returns the
    bytes and each row's starting offset."""
    data, offsets = [], []
    for row in grid:
        offsets.append(len(data))
        x = 0
        while x < len(row):
            v = row[x]
            n = 1
            while x + n < len(row) and row[x + n] == v and n < 255:
                n += 1
            data += [n, v]
            x += n
    return data, offsets


# ------------------------------------------------------------ editing tiles

def shade_colour(cfg, shade):
    """An RGBA colour that converts back to exactly this shade under the
    tileset's thresholds: transparent for the lightest, greys between the
    cut-offs for the rest."""
    t0, t1, t2 = cfg.get("thresholds", [0.72, 0.5, 0.3])
    if shade == 0:
        return (0, 0, 0, 0)
    y = {1: (t0 + t1) / 2, 2: (t1 + t2) / 2, 3: t2 / 2}[shade]
    g = round(y * 255)
    return (g, g, g, 255)


def _image(path):
    path = Path(path)
    cfg = json.loads(path.read_text())
    img = path.parent / cfg["image"]
    w, h, px = read_png(img)
    return cfg, img, w, h, px


def set_tile(index, shades, path=DEFAULT):
    """Repaint tile `index` with 64 shades (0 lightest .. 3 darkest)."""
    cfg, img, w, h, px = _image(path)
    cols = w // 8
    if not 0 <= index < cols * (h // 8):
        raise ValueError(f"no tile {index}")
    if len(shades) != 64 or any(s not in (0, 1, 2, 3) for s in shades):
        raise ValueError("a tile is 64 shades of 0-3")
    tx, ty = index % cols, index // cols
    for y in range(8):
        for x in range(8):
            px[ty * 8 + y][tx * 8 + x] = shade_colour(cfg, shades[y * 8 + x])
    write_png(img, w, h, px)


def add_row(path=DEFAULT):
    """Add a row of blank tiles at the bottom (existing numbers don't move)."""
    cfg, img, w, h, px = _image(path)
    px += [[(0, 0, 0, 0)] * w for _ in range(8)]
    write_png(img, w, h + 8, px)
