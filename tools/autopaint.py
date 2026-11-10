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

Where you've painted a cell shape before (a wall with exactly these open
neighbours, say), the tile you used most often for it wins over these
rules, and a tile you painted onto that shape by hand in the editor (the
latest one, saved as autopaint.taught in the tileset settings) wins over
both: see learn().

Floor ('.' and the start) gets "floor", exits "exit", doors 'D' "door",
vents 'V' "vent". A one-tile gap in a horizontal wall (wall left and
right, open above and below) is painted as a door but keeps its floor
behaviour. Other building blocks keep their built-in look.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tileset as ts   # noqa: E402


FACE = "face"     # stands for the repeating face tiles (cfg["face"])

# Neighbour bits for pattern(): N, NE, E, SE, S, SW, W, NW.
NEIGHBOURS = [(0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1)]


def _grid_fns(rows):
    h, w = len(rows), len(rows[0])

    def wall(x, y):
        return not (0 <= x < w and 0 <= y < h) or rows[y][x] == "#"

    def opn(x, y):
        return not wall(x, y)

    return h, w, wall, opn


def pattern(rows, x, y):
    """(kind of cell, which of its 8 neighbours are open) for a cell:
    the shape that decides how it's drawn. Outside the map counts as wall."""
    _, _, wall, _ = _grid_fns(rows)
    bits = 0
    for i, (dx, dy) in enumerate(NEIGHBOURS):
        if not wall(x + dx, y + dy):
            bits |= 1 << i
    c = rows[y][x]
    return ("." if c == "P" else c), bits


def rule(rows, x, y, cfg):
    """The built-in choice for one cell (see the module docstring)."""
    _, _, wall, opn = _grid_fns(rows)
    c = rows[y][x]
    if c != "#":
        doorway = (c in ".P" and wall(x - 1, y) and wall(x + 1, y)
                   and opn(x, y - 1) and opn(x, y + 1))
        if doorway or c == "D":
            return cfg["door"]
        if c in ".P":
            return cfg["floor"]
        return {"E": cfg["exit"], "V": cfg["vent"]}.get(c)
    if opn(x, y + 1):
        return FACE
    for (dx, dy), name in [((1, 0), "left"), ((-1, 0), "right"),
                           ((1, 1), "top_left"), ((-1, 1), "top_right"),
                           ((0, -1), "bottom"),
                           ((1, -1), "bottom_left"), ((-1, -1), "bottom_right")]:
        if opn(x + dx, y + dy):
            return cfg[name]
    return None


def learn(levels, cfg, skip=()):
    """{pattern: tile} from painted maps: for every cell shape you've
    painted, the tile you used most often (FACE for the face tiles on
    walls with open floor below). Shapes never painted aren't included.
    skip: (level, map, x, y) cells to leave out, e.g. ones just edited,
    whose art is stale."""
    from collections import Counter
    counts = {}
    faces = set(cfg["face"])
    skip = set(skip)
    for li, lv in enumerate(levels):
        for mi, m in enumerate(lv.get("maps", [])):
            if not m.get("art"):
                continue
            rows, art = m["map"], ts.parse_art(m["art"])
            for y in range(len(rows)):
                for x in range(len(rows[0])):
                    if (li, mi, x, y) in skip:
                        continue
                    key = pattern(rows, x, y)
                    tile = art[y][x]
                    if key[0] == "#" and key[1] & (1 << 4) and tile in faces:
                        tile = FACE
                    counts.setdefault(key, Counter())[tile] += 1
    learned = {}
    for key, c in counts.items():
        (best, n), *rest = c.most_common(2) + [(None, 0)]
        if not rest or rest[0][1] < n:          # a clear favourite only
            learned[key] = best
    learned.update(taught(cfg))                 # taught shapes always win
    return learned


def taught_key(key):
    """JSON key for a pattern: kind of cell + its open-neighbour bits."""
    return f"{key[0]}{key[1]}"


def taught(cfg):
    """{pattern: tile} you taught by painting a cell by hand (see the
    editor), stored in the tileset settings under autopaint.taught."""
    out = {}
    for k, tile in cfg.get("taught", {}).items():
        out[(k[0], int(k[1:]))] = tile
    return out


def autopaint(rows, cfg, learned=None):
    """Art grid (tileset tile or None per cell) for a map's rows: learned
    choices where this shape has been painted before, the rules elsewhere,
    then the face tiles laid out along each wall."""
    h, w, wall, opn = _grid_fns(rows)
    choice = [[None] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            key = pattern(rows, x, y)
            choice[y][x] = learned[key] if learned and key in learned else rule(rows, x, y, cfg)
    face = cfg["face"]
    grid = [[None] * w for _ in range(h)]
    for y in range(h):
        run_start = None          # where the current run of face tiles began
        for x in range(w):
            v = choice[y][x]
            if v == FACE:
                if run_start is None:
                    run_start = x
                grid[y][x] = face[(x - run_start) % len(face)]
                continue
            grid[y][x] = v
            c = rows[y][x]
            doorway = (c in ".P" and wall(x - 1, y) and wall(x + 1, y)
                       and opn(x, y - 1) and opn(x, y + 1))
            if not doorway:               # a doorway doesn't break the pattern
                run_start = None
    return grid


def autopaint_rows(rows, tileset_cfg=None, learned=None):
    """Art rows (as stored in levels.json) for a map."""
    cfg = (tileset_cfg or ts.load())["autopaint"]
    return ts.format_art(autopaint(rows, cfg, learned))


def autotile(rows, art_rows, changed, cfg, learned=None):
    """Repaint only around edited cells: every cell within one tile of a
    changed cell, plus the whole run of any wall face that passes through
    them (so the face pattern stays even). Everything else keeps its art.
    Returns new art rows."""
    h, w = len(rows), len(rows[0])
    fresh = autopaint(rows, cfg, learned)
    if not art_rows:
        return ts.format_art(fresh)
    art = ts.parse_art(art_rows)
    if len(art) != h or len(art[0]) != w:      # stale art: start over
        return ts.format_art(fresh)
    faces = set(cfg["face"])
    touch = set()
    for cx, cy in changed:
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if 0 <= cx + dx < w and 0 <= cy + dy < h:
                    touch.add((cx + dx, cy + dy))
    for x, y in list(touch):                    # extend along face runs
        if fresh[y][x] in faces and rows[y][x] == "#":
            for step in (-1, 1):
                k = x + step
                while 0 <= k < w and (fresh[y][k] in faces or rows[y][k] != "#") \
                        and fresh[y][k] is not None and rows[y][k] in "#.P":
                    touch.add((k, y))
                    if rows[y][k] != "#" and fresh[y][k] != cfg["door"]:
                        break
                    k += step
    for x, y in touch:
        art[y][x] = fresh[y][x]
    return ts.format_art(art)


def main():
    import gen_levels
    levels = gen_levels.load()
    cfg = ts.load()
    learned = learn(levels, cfg["autopaint"])     # keep your painted style
    for lv in levels:
        for m in lv["maps"]:
            m["art"] = autopaint_rows(m["map"], cfg, learned)
    errs = gen_levels.save(levels)
    if errs:
        print("\n".join(errs), file=sys.stderr)
        sys.exit(1)
    print(f"repainted {sum(len(lv['maps']) for lv in levels)} maps")


if __name__ == "__main__":
    main()
