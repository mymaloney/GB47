#!/usr/bin/env python3
"""Generate src/tiles.c and src/tiles.h from the ASCII art below.

Run after editing art:   python3 tools/gen_tiles.py

Pixel characters ->  Game Boy colour index (0 = lightest, 3 = darkest)
    '.' 0    ':' 1    'x' 2    '#' 3
For sprites, colour 0 is transparent.

This is a stopgap until you draw real art in Aseprite/GBTD and convert it
with GBDK's png2asset.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PIX = {".": 0, ":": 1, "x": 2, "#": 3}

# ---------------------------------------------------------------- background
BKG = [
    ("TILE_BLANK", [
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    ]),
    ("TILE_FLOOR", [
        "........",
        "........",
        "........",
        "...:....",
        "........",
        "........",
        "........",
        ".......:",
    ]),
    ("TILE_WALL", [
        "########",
        "xxx#xxxx",
        "xxx#xxxx",
        "########",
        "x#xxxx#x",
        "x#xxxx#x",
        "########",
        "xxx#xxxx",
    ]),
    ("TILE_EXIT", [
        "########",
        "#......#",
        "#.####.#",
        "#.#..#.#",
        "#.#..#.#",
        "#.####.#",
        "#......#",
        "########",
    ]),
    ("TILE_SEEN", [   # floor inside a guard's vision cone
        "::::::::",
        ":x:::x::",
        "::::::::",
        "::::::::",
        "::::::::",
        "::x:::x:",
        "::::::::",
        "::::::::",
    ]),
    ("TILE_EXIT_SEEN", [
        "########",
        "#::::::#",
        "#:####:#",
        "#:#::#:#",
        "#:#::#:#",
        "#:####:#",
        "#::::::#",
        "########",
    ]),
    ("TILE_AIM", [        # floor you can throw to (shown while aiming)
        "........",
        "........",
        "........",
        "...xx...",
        "...xx...",
        "........",
        "........",
        "........",
    ]),
    ("TILE_AIM_SEEN", [   # throwable AND inside a vision cone
        "::::::::",
        ":x:::x::",
        "::::::::",
        ":::##:::",
        ":::##:::",
        "::x:::x:",
        "::::::::",
        "::::::::",
    ]),
    ("TILE_WIN_EDGE", [   # top border of the inventory window
        "........",
        "........",
        "########",
        "xxxxxxxx",
        "........",
        "........",
        "........",
        "........",
    ]),
]

# 5x7 font, placed at column 1 of each 8x8 tile.
FONT = {
    "0": [".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."],
    "1": ["..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."],
    "2": [".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"],
    "3": ["####.", "....#", "....#", ".###.", "....#", "....#", "####."],
    "4": ["...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."],
    "5": ["#####", "#....", "####.", "....#", "....#", "#...#", ".###."],
    "6": ["..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."],
    "7": ["#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."],
    "8": [".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."],
    "9": [".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."],
    "A": [".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    "B": ["####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."],
    "C": [".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."],
    "D": ["####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."],
    "E": ["#####", "#....", "#....", "####.", "#....", "#....", "#####"],
    "F": ["#####", "#....", "#....", "####.", "#....", "#....", "#...."],
    "G": [".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"],
    "H": ["#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    "I": [".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."],
    "J": ["..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."],
    "K": ["#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"],
    "L": ["#....", "#....", "#....", "#....", "#....", "#....", "#####"],
    "M": ["#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"],
    "N": ["#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#"],
    "O": [".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."],
    "P": ["####.", "#...#", "#...#", "####.", "#....", "#....", "#...."],
    "Q": [".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"],
    "R": ["####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"],
    "S": [".####", "#....", "#....", ".###.", "....#", "....#", "####."],
    "T": ["#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."],
    "U": ["#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."],
    "V": ["#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."],
    "W": ["#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."],
    "X": ["#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"],
    "Y": ["#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."],
    "Z": ["#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"],
    ":": [".....", "..#..", "..#..", ".....", "..#..", "..#..", "....."],
    "!": ["..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."],
    "-": [".....", ".....", ".....", ".###.", ".....", ".....", "....."],
    ".": [".....", ".....", ".....", ".....", ".....", ".##..", ".##.."],
    ">": [".#...", "..#..", "...#.", "....#", "...#.", "..#..", ".#..."],
    "?": [".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."],
}
FONT_ORDER = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:!-.>?"

# ------------------------------------------------------------------- sprites
# Guards differ only by where the eyes are, so facing reads at a glance
# (the vision cone shows it too).
def guard(eye_row):
    return [
        "..####..",
        ".######.",
        eye_row,
        ".::::::.",
        "..xxxx..",
        ".xxxxxx.",
        ".xxxxxx.",
        ".##..##.",
    ]

def rotate_ccw(rows):
    """Rotate 8x8 art 90 degrees counter-clockwise (top edge -> left edge)."""
    return ["".join(rows[c][7 - r] for c in range(8)) for r in range(8)]


SPRITES = [
    ("SPR_PLAYER", [
        "...##...",
        "..####..",
        "..#::#..",
        "...##...",
        ".#.##.#.",
        "..####..",
        "...##...",
        "..#..#..",
    ]),
    ("SPR_GUARD_UP",    guard(".######.")),
    ("SPR_GUARD_RIGHT", guard(".:::#:#.")),
    ("SPR_GUARD_DOWN",  guard(".:#::#:.")),
    ("SPR_GUARD_LEFT",  guard(".#:#:::.")),
    ("SPR_GUARD_SUBDUED", guard(".:x::x:.")),   # dazed, mid-takedown
    ("SPR_BODY", rotate_ccw(guard(".:-::-:.".replace("-", "x")))),  # lying down
    ("SPR_CURSOR", [      # aiming at a valid target
        "##....##",
        "#......#",
        "........",
        "...##...",
        "...##...",
        "........",
        "#......#",
        "##....##",
    ]),
    ("SPR_CURSOR_BAD", [  # aiming somewhere you can't throw
        "#......#",
        ".#....#.",
        "..#..#..",
        "...##...",
        "...##...",
        "..#..#..",
        ".#....#.",
        "#......#",
    ]),
    ("SPR_NOISE", [       # where a thrown item landed / guard is headed
        "........",
        "..#..#..",
        ".#....#.",
        "#..xx..#",
        "#..xx..#",
        ".#....#.",
        "..#..#..",
        "........",
    ]),
]


def encode(rows, name):
    assert len(rows) == 8, name
    out = []
    for r in rows:
        assert len(r) == 8, f"{name}: row {r!r} is not 8 wide"
        lo = hi = 0
        for i, ch in enumerate(r):
            v = PIX[ch]
            lo |= (v & 1) << (7 - i)
            hi |= ((v >> 1) & 1) << (7 - i)
        out += [lo, hi]
    return out


def font_tile(glyph):
    rows = ["." + g + ".." for g in glyph] + ["........"]
    return rows


def c_array(name, tiles):
    lines = [f"const uint8_t {name}[] = {{"]
    for label, data in tiles:
        hexes = ", ".join(f"0x{b:02X}" for b in data)
        lines.append(f"    /* {label} */ {hexes},")
    lines.append("};")
    return "\n".join(lines)


def main():
    bkg = [(n, encode(r, n)) for n, r in BKG]
    font_base = len(bkg)
    bkg += [(f"'{c}'", encode(font_tile(FONT[c]), c)) for c in FONT_ORDER]
    spr = [(n, encode(r, n)) for n, r in SPRITES]

    h = ["/* Generated by tools/gen_tiles.py - do not edit by hand. */",
         "#ifndef TILES_H", "#define TILES_H", "", "#include <stdint.h>", ""]
    for i, (n, _) in enumerate(BKG):
        h.append(f"#define {n} {i}")
    h += ["",
          f"#define FONT_BASE   {font_base}   /* '0'-'9', then 'A'-'Z' */",
          f"#define FONT_COLON  {font_base + FONT_ORDER.index(':')}",
          f"#define FONT_BANG   {font_base + FONT_ORDER.index('!')}",
          f"#define FONT_DASH   {font_base + FONT_ORDER.index('-')}",
          f"#define FONT_PERIOD {font_base + FONT_ORDER.index('.')}",
          f"#define FONT_GT     {font_base + FONT_ORDER.index('>')}",
          f"#define FONT_QMARK  {font_base + FONT_ORDER.index('?')}",
          f"#define BKG_TILE_COUNT {len(bkg)}", ""]
    for i, (n, _) in enumerate(SPRITES):
        h.append(f"#define {n} {i}")
    h += [f"#define SPRITE_TILE_COUNT {len(spr)}", "",
          "extern const uint8_t bkg_tile_data[];",
          "extern const uint8_t sprite_tile_data[];", "", "#endif", ""]

    c = ["/* Generated by tools/gen_tiles.py - do not edit by hand. */",
         '#include "tiles.h"', "",
         c_array("bkg_tile_data", bkg), "",
         c_array("sprite_tile_data", spr), ""]

    (ROOT / "src" / "tiles.h").write_text("\n".join(h))
    (ROOT / "src" / "tiles.c").write_text("\n".join(c))
    print(f"wrote {len(bkg)} background tiles, {len(spr)} sprite tiles")


if __name__ == "__main__":
    main()
