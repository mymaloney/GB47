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
MESH = [
    "x.x.x.x.",
    ".x...x..",
    "x.x.x.x.",
    "...x...x",
    "x.x.x.x.",
    ".x...x..",
    "x.x.x.x.",
    "...x...x",
]
WATCH = [   # same pattern as TILE_WATCH below
    "........",
    ".:...:..",
    "........",
    "...:...:",
    "........",
    ".:...:..",
    "........",
    "...:...:",
]


def overlay(top, bottom):
    """Pixels of top that aren't '.', drawn over bottom."""
    return ["".join(a if a != "." else b for a, b in zip(r1, r2)) for r1, r2 in zip(top, bottom)]


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
    ("TILE_WALL", ["########"] * 8),   # flat black: the void outside rooms
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
    ("TILE_WATCH", WATCH),  # floor only a blind guard watches (bodies, suspicious acts)
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
    # --- building blocks (see the table in src/game.h) ---
    ("TILE_DOOR", [       # walk through; blocks sight and throws
        "########",
        "#xxxxxx#",
        "#x::::x#",
        "#xxxxxx#",
        "#xxxx:x#",
        "#x::::x#",
        "#xxxxxx#",
        "#xxxxxx#",
    ]),
    ("TILE_WINDOW", [     # see through; blocks walking and throws
        "########",
        "#:..#..#",
        "#.:.#..#",
        "########",
        "#...#:.#",
        "#...#.:#",
        "#...#..#",
        "########",
    ]),
    ("TILE_RAILING", [    # see and throw over; blocks walking
        "........",
        "xxxxxxxx",
        "#..#..#.",
        "#..#..#.",
        "xxxxxxxx",
        "#..#..#.",
        "#..#..#.",
        "........",
    ]),
    ("TILE_SECURITY", [   # guards only; blocks sight and throws
        "########",
        "#x#x#x##",
        "##x#x#x#",
        "#x#x#x##",
        "########",
        "#xx::xx#",
        "#xx::xx#",
        "########",
    ]),
    ("TILE_CURTAIN", [    # walk in and hide; throws pass
        "########",
        "x:x:x:x:",
        "x:x:x:x:",
        "x:x:x:x:",
        "x:x:x:x:",
        "x:x:x:x:",
        ":x:x:x:x",
        "x.x.x.x.",
    ]),
    ("TILE_MESH", MESH),  # walk and see through; blocks throws
    ("TILE_MESH_SEEN", [r.replace(".", ":") for r in MESH]),
    ("TILE_MESH_WATCH", overlay(MESH, WATCH)),
    ("TILE_VENT", [       # player only; hides you; blocks sight and throws
        "########",
        "#......#",
        "########",
        "#......#",
        "########",
        "#......#",
        "########",
        "#......#",
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
# (the vision cone shows it too). Each uniform style changes hat and body.
GUARD_STYLES = {
    # name: (hat row 0, hat row 1, body row 4, body rows 5-6)
    "GUARD":   ("..####..", ".######.", "..xxxx..", ".xxxxxx."),
    "OFFICER": ("..####..", "#::::::#", "..#xx#..", ".#x##x#."),
}

def guard(eye_row, style="GUARD"):
    h0, h1, b4, b5 = GUARD_STYLES[style]
    return [h0, h1, eye_row, ".::::::.", b4, b5, b5, ".##..##."]


def rotate_ccw(rows):
    """Rotate 8x8 art 90 degrees counter-clockwise (top edge -> left edge)."""
    return ["".join(rows[c][7 - r] for c in range(8)) for r in range(8)]


def guard_set(style):
    """6 tiles per style: facing up/right/down/left, subdued, body."""
    p = "SPR_" + style + "_"
    return [
        (p + "UP",      guard(".######.", style)),
        (p + "RIGHT",   guard(".:::#:#.", style)),
        (p + "DOWN",    guard(".:#::#:.", style)),
        (p + "LEFT",    guard(".#:#:::.", style)),
        (p + "SUBDUED", guard(".:x::x:.", style)),            # mid-takedown
        (p + "BODY",    rotate_ccw(guard(".:x::x:.", style))),  # lying down
    ]


def player(hat0, hat1, body):
    return [hat0, hat1, "..#::#..", "...##...", ".#" + body + "#.",
            "..####..", "...##...", "..#..#.."]


# Order of the player tiles must match DISGUISE_* in game.h.
SPRITES = [
    ("SPR_PLAYER_NONE",    player("...##...", "..####..", ".##.")),
    ("SPR_PLAYER_GUARD",   player("..####..", ".######.", "xxxx")),
    ("SPR_PLAYER_OFFICER", player("..####..", "#::::::#", "x##x")),
] + guard_set("GUARD") + guard_set("OFFICER") + [
    ("SPR_BODY_STRIPPED", rotate_ccw([   # uniform taken
        "...::...", "..::::..", ".:x::x:.", ".::::::.",
        "..::::..", ".:::::..", ".::::::.", ".::..::.",
    ])),
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
    h += ["#define SPR_PLAYER  SPR_PLAYER_NONE   /* + DISGUISE_* */",
          "#define SPR_GUARD_STYLE_TILES 6  /* up right down left subdued body */"]
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
