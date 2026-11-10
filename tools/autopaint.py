#!/usr/bin/env python3
"""Auto-paint a map's art from its behaviour layer, with a tileset's
"autopaint" tiles (tilesets/<name>.json).

    python3 tools/autopaint.py            repaint every map of every level

Rooms are drawn as thin outlines on a black void. For each wall ('#'),
looking at which neighbours are open (anything that isn't wall):

    1. open below            -> face tiles ("face", repeating from the
                                left end of each run of face cells)
    2. open to the right     -> "left" side      (the room's left wall)
       open to the left      -> "right" side
    3. open below-right      -> "top_left" corner (also when the floor
       open below-left       -> "top_right" corner   above is open)
    4. open above            -> "bottom" edge
    5. open above-right      -> "bottom_left" corner
       open above-left       -> "bottom_right" corner
    6. nothing open nearby   -> left unpainted: the built-in wall, flat black

Floor ('.' and the start) gets "floor", exits "exit", doors 'D' "door",
vents 'V' "vent". A one-tile gap in a horizontal wall (wall left and
right, open above and below) is painted as a door but keeps its floor
behaviour. Other building blocks keep their built-in look.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tileset as ts   # noqa: E402


def autopaint(rows, cfg):
    """Art grid (tileset tile or None per cell) for a map's rows."""
    h, w = len(rows), len(rows[0])

    def wall(x, y):
        return not (0 <= x < w and 0 <= y < h) or rows[y][x] == "#"

    def opn(x, y):
        return 0 <= x < w and 0 <= y < h and rows[y][x] != "#"

    grid = [[None] * w for _ in range(h)]
    face = cfg["face"]
    for y in range(h):
        run_start = None          # where the current run of face tiles began
        for x in range(w):
            c = rows[y][x]
            if c != "#":
                doorway = (c in ".P" and wall(x - 1, y) and wall(x + 1, y)
                           and opn(x, y - 1) and opn(x, y + 1))
                if not doorway:
                    run_start = None   # a doorway doesn't break the face pattern
                if doorway or c == "D":
                    grid[y][x] = cfg["door"]
                elif c in ".P":
                    grid[y][x] = cfg["floor"]
                elif c == "E":
                    grid[y][x] = cfg["exit"]
                elif c == "V":
                    grid[y][x] = cfg["vent"]
                continue
            if opn(x, y + 1):
                if run_start is None:
                    run_start = x
                grid[y][x] = face[(x - run_start) % len(face)]
                continue
            run_start = None
            if opn(x + 1, y):
                grid[y][x] = cfg["left"]
            elif opn(x - 1, y):
                grid[y][x] = cfg["right"]
            elif opn(x + 1, y + 1):
                grid[y][x] = cfg["top_left"]
            elif opn(x - 1, y + 1):
                grid[y][x] = cfg["top_right"]
            elif opn(x, y - 1):
                grid[y][x] = cfg["bottom"]
            elif opn(x + 1, y - 1):
                grid[y][x] = cfg["bottom_left"]
            elif opn(x - 1, y - 1):
                grid[y][x] = cfg["bottom_right"]
    return grid


def autopaint_rows(rows, tileset_cfg=None):
    """Art rows (as stored in levels.json) for a map."""
    cfg = (tileset_cfg or ts.load())["autopaint"]
    return ts.format_art(autopaint(rows, cfg))


def main():
    import gen_levels
    levels = gen_levels.load()
    cfg = ts.load()
    for lv in levels:
        for m in lv["maps"]:
            m["art"] = autopaint_rows(m["map"], cfg)
    errs = gen_levels.save(levels)
    if errs:
        print("\n".join(errs), file=sys.stderr)
        sys.exit(1)
    print(f"repainted {sum(len(lv['maps']) for lv in levels)} maps")


if __name__ == "__main__":
    main()
