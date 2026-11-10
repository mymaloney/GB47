#!/usr/bin/env python3
"""Level data: levels/levels.json -> src/levels.c and src/levels.h.

    python3 tools/gen_levels.py          (or: make levels)

levels/levels.json is the source of truth; edit it with the level editor
(make editor) or by hand. The generated C files are committed so the ROM
builds without Python, but don't edit them directly.

JSON format, one object per level:

    {
      "name":  "FIRST STEPS",              max 12 chars, HUD font only
      "notes": "Shown as a comment in levels.c",
      "items": {"stone": 0, "bottle": 0},  what the player starts with
      "maps": [{                           1-4 maps; the player starts in
                                           the first
        "map": [rows of equal length],     '#' wall '.' floor 'E' exit
                                           'P' player start, and the
                                           building blocks in TILES below.
                                           Whole screens of 20x16: 1-3
                                           across, 1-3 down, at most 2048
                                           tiles (so not 3x3)
        "art": ["0a0a....", ...],          optional painted look: two hex
                                           digits per cell = tileset tile
                                           (tilesets/), ".." = built-in
                                           graphic for the cell's kind
        "guards": [{
          "facing":   "right",             up/right/down/left
          "route":    [[1, 5], [18, 5]],   patrol loop, starts at the first
                                           point; one point = sentry
          "uniform":  "none",              none/guard/officer (from body)
          "attitude": null                 or e.g. "HBE": hostile/enforcer/
                                           blind to own clothes, GUARD,
                                           OFFICER; null = enforcer to all
        }]
      }],
      "links": [{                          joins two maps' edges (both ways)
        "from": 0, "edge": "right",        map index and which of its edges
        "to": 1,                           the map beyond that edge
        "offset": 0                        where the other map's row 0 (or
                                           column 0, for up/down) lines up
                                           along this edge
      }]
    }

Walking off a linked edge enters the other map at the matching tile, so
openings on both sides must line up (validate_level checks this).
"""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tileset as ts   # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
JSON_PATH = ROOT / "levels" / "levels.json"

# Must match src/game.h and src/world.h
VIEW_W, VIEW_H = 20, 16
MAP_MAX_W, MAP_MAX_H, MAP_MAX_CELLS = 60, 48, 2048
MAX_GUARDS, MAX_WAYPOINTS = 4, 6
MAX_MAPS = 4
NAME_MAX = 12
DIRS = ["up", "right", "down", "left"]
OPPOSITE = {"up": "down", "down": "up", "left": "right", "right": "left"}
UNIFORMS = ["none", "guard", "officer"]
ITEMS = ["stone", "bottle"]
FONT_CHARS = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 :!-.>?")


def art_slots_available():
    """Background tiles left for painted art: 256 minus the built-in ones
    (src/tiles.h)."""
    m = re.search(r"#define BKG_TILE_COUNT (\d+)", (ROOT / "src" / "tiles.h").read_text())
    return 256 - int(m.group(1))


_tileset = None


def tileset():
    global _tileset
    if _tileset is None:
        _tileset = ts.load()
    return _tileset

# Map characters: (name, blocks player+bodies, guards, sight, throws).
# Must match the table in src/game.h.
TILES = {
    "#": ("wall",          True,  True,  True,  True),
    ".": ("floor",         False, False, False, False),
    "P": ("player start",  False, False, False, False),
    "E": ("exit",          False, False, False, False),
    "D": ("door",          False, False, True,  True),
    "W": ("window",        True,  True,  False, True),
    "R": ("railing",       True,  True,  False, False),
    "S": ("security door", True,  False, True,  True),
    "C": ("curtain",       False, False, True,  False),
    "M": ("mesh",          False, False, False, True),
    "V": ("vent",          False, True,  True,  True),
}


def upgrade(lv):
    """Old one-map levels had map/guards at the top level."""
    if "maps" not in lv:
        lv = dict(lv)
        lv["maps"] = [{"map": lv.pop("map", []), "guards": lv.pop("guards", [])}]
        lv.setdefault("links", [])
    lv.setdefault("links", [])
    lv.setdefault("items", {})
    return lv


def map_size(m):
    rows = m.get("map", [])
    return (len(rows[0]) if rows else 0), len(rows)


def validate_map(m):
    """Problems with one map on its own (size, tiles, guards)."""
    errs = []
    rows = m.get("map", [])
    mh = len(rows)
    mw = len(rows[0]) if rows else 0
    if any(len(r) != mw for r in rows):
        return ["All map rows must be the same length."]
    if not (VIEW_W <= mw <= MAP_MAX_W and VIEW_H <= mh <= MAP_MAX_H):
        return [f"Map is {mw}x{mh}; it must be {VIEW_W}-{MAP_MAX_W} wide and "
                f"{VIEW_H}-{MAP_MAX_H} tall."]
    if mw % VIEW_W or mh % VIEW_H:
        return [f"Map is {mw}x{mh}; it must be whole screens "
                f"({VIEW_W} wide x {VIEW_H} tall each)."]
    if mw * mh > MAP_MAX_CELLS:
        return [f"Map has {mw * mh} tiles; the Game Boy has room for "
                f"{MAP_MAX_CELLS} (e.g. 40x48 or 60x32)."]
    bad_chars = sorted({c for r in rows for c in r} - set(TILES))
    if bad_chars:
        return [f"Map has unknown tiles: {' '.join(bad_chars)} "
                f"(allowed: {' '.join(TILES)})."]
    starts = [(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c == "P"]

    def guard_blocked(x, y):
        """Off the map, or a tile guards can't walk on. Returns its name."""
        if not (0 <= x < mw and 0 <= y < mh):
            return "the edge of the map"
        name, _, blocks_guard, _, _ = TILES[rows[y][x]]
        return name if blocks_guard else None

    guards = m.get("guards", [])
    if len(guards) > MAX_GUARDS:
        errs.append(f"At most {MAX_GUARDS} guards (has {len(guards)}).")
    seen_starts = {}
    for i, g in enumerate(guards):
        tag = f"Guard {i + 1}"
        route = g.get("route", [])
        if not 1 <= len(route) <= MAX_WAYPOINTS:
            errs.append(f"{tag}: route needs 1-{MAX_WAYPOINTS} points (has {len(route)}).")
            continue
        if g.get("facing") not in DIRS:
            errs.append(f"{tag}: facing must be one of {', '.join(DIRS)}.")
        if g.get("uniform", "none") not in UNIFORMS:
            errs.append(f"{tag}: uniform must be one of {', '.join(UNIFORMS)}.")
        att = g.get("attitude")
        if att is not None and (len(att) != len(UNIFORMS) or set(att) - set("HEB")):
            errs.append(f"{tag}: attitude must be {len(UNIFORMS)} letters of H, E, B.")
        for w, (x, y) in enumerate(route):
            what = guard_blocked(x, y)
            if what:
                errs.append(f"{tag}: point {w + 1} ({x},{y}) is on a {what}, "
                            "which guards can't stand on.")
        start = tuple(route[0])
        if start in seen_starts:
            errs.append(f"{tag} starts on the same tile as guard {seen_starts[start] + 1}.")
        seen_starts[start] = i
        if starts and start == starts[0]:
            errs.append(f"{tag} starts on the player.")
        if len(route) > 1:
            for w in range(len(route)):
                (ax, ay), (bx, by) = route[w], route[(w + 1) % len(route)]
                leg = f"{tag}: leg {w + 1}->{(w + 1) % len(route) + 1}"
                if (ax, ay) == (bx, by):
                    errs.append(f"{leg} goes nowhere (repeated point).")
                elif ax != bx and ay != by:
                    errs.append(f"{leg} isn't a straight line; add a corner point.")
                else:
                    sx = (bx > ax) - (bx < ax)
                    sy = (by > ay) - (by < ay)
                    x, y = ax, ay
                    while (x, y) != (bx, by):
                        x, y = x + sx, y + sy
                        what = guard_blocked(x, y)
                        if what:
                            errs.append(f"{leg} walks through a {what} at ({x},{y}).")
                            break
    errs += validate_art(m)
    return errs


def art_usage(m):
    """{tileset tile: set of map chars it's painted on} for one map."""
    used = {}
    if not m.get("art"):
        return used
    for row, arow in zip(m["map"], ts.parse_art(m["art"])):
        for ch, v in zip(row, arow):
            if v is not None:
                used.setdefault(v, set()).add(ch)
    return used


def validate_art(m):
    art = m.get("art")
    if not art:
        return []
    rows = m["map"]
    if len(art) != len(rows) or any(len(a) != 2 * len(rows[0]) for a in art):
        return ["Art layer doesn't match the map's size."]
    if any(not re.fullmatch(r"([0-9a-f]{2}|\.\.)*", a) for a in art):
        return ["Art layer has invalid entries (two hex digits or '..' per cell)."]
    n = len(tileset()["tiles"])
    used = art_usage(m)
    bad = sorted(v for v in used if v >= n)
    if bad:
        return [f"Art uses tile {bad[0]}, but the tileset has {n} tiles."]
    need = sum(ts.slots_for(chars) for chars in used.values())
    have = art_slots_available()
    if need > have:
        return [f"Art needs {need} Game Boy tile slots; there's room for {have}. "
                "Use fewer different tiles, especially on floor (5 slots each)."]
    return []


def edge_tiles(m, edge):
    """(position along the edge, char) for each tile on one edge of a map."""
    rows = m["map"]
    w, h = map_size(m)
    if edge == "left":
        return [(y, rows[y][0]) for y in range(h)]
    if edge == "right":
        return [(y, rows[y][w - 1]) for y in range(h)]
    if edge == "up":
        return [(x, rows[0][x]) for x in range(w)]
    return [(x, rows[h - 1][x]) for x in range(w)]


def walkable(ch):
    return not TILES[ch][1]


def link_table(lv):
    """{(map, edge): (other map, offset)} for both directions of every link."""
    out = {}
    for ln in lv.get("links", []):
        a, e, b, off = ln["from"], ln["edge"], ln["to"], ln.get("offset", 0)
        out[(a, e)] = (b, off)
        out[(b, OPPOSITE[e])] = (a, -off)
    return out


def validate_links(lv):
    errs = []
    maps = lv["maps"]
    used = {}
    for n, ln in enumerate(lv.get("links", [])):
        tag = f"Link {n + 1}"
        a, e, b = ln.get("from"), ln.get("edge"), ln.get("to")
        off = ln.get("offset", 0)
        if e not in DIRS:
            errs.append(f"{tag}: edge must be one of {', '.join(DIRS)}.")
            continue
        if not (isinstance(a, int) and isinstance(b, int) and
                0 <= a < len(maps) and 0 <= b < len(maps)):
            errs.append(f"{tag}: refers to a map that doesn't exist.")
            continue
        if a == b:
            errs.append(f"{tag}: a map can't link to itself.")
            continue
        if not isinstance(off, int):
            errs.append(f"{tag}: offset must be a whole number.")
            continue
        for key in ((a, e), (b, OPPOSITE[e])):
            if key in used:
                errs.append(f"Map {key[0] + 1}'s {key[1]} edge is linked twice "
                            f"(links {used[key] + 1} and {n + 1}).")
            used[key] = n
        # the two edges must overlap, and openings must line up
        span_a = map_size(maps[a])[1] if e in ("left", "right") else map_size(maps[a])[0]
        span_b = map_size(maps[b])[1] if e in ("left", "right") else map_size(maps[b])[0]
        if not -span_b < off < span_a:
            errs.append(f"{tag}: with offset {off}, map {a + 1}'s {e} edge and map "
                        f"{b + 1}'s {OPPOSITE[e]} edge don't touch.")
            continue
        here = dict(edge_tiles(maps[a], e))
        there = dict(edge_tiles(maps[b], OPPOSITE[e]))
        for pos, ch in here.items():
            other = there.get(pos - off)
            if walkable(ch) and (other is None or not walkable(other)):
                errs.append(f"{tag}: the opening at {e} edge position {pos} of map {a + 1} "
                            f"leads into {'nothing' if other is None else 'a ' + TILES[other][0]} "
                            f"in map {b + 1}.")
        for pos, ch in there.items():
            other = here.get(pos + off)
            if walkable(ch) and (other is None or not walkable(other)):
                errs.append(f"{tag}: the opening at {OPPOSITE[e]} edge position {pos} of map "
                            f"{b + 1} leads into "
                            f"{'nothing' if other is None else 'a ' + TILES[other][0]} "
                            f"in map {a + 1}.")
    # every map reachable from the first
    reach, todo = {0}, [0]
    while todo:
        m = todo.pop()
        for (src, _), (dst, _) in link_table(lv).items():
            if src == m and dst not in reach:
                reach.add(dst)
                todo.append(dst)
    for m in range(len(maps)):
        if m not in reach:
            errs.append(f"Map {m + 1} isn't linked to the rest of the level.")
    return errs


def validate_level(lv):
    """Return a list of human-readable problems (empty = OK)."""
    lv = upgrade(lv)
    errs = []
    name = lv.get("name", "")
    if not name.strip():
        errs.append("Level needs a name.")
    if len(name) > NAME_MAX:
        errs.append(f"Name is {len(name)} characters; the HUD fits {NAME_MAX}.")
    bad = sorted(set(name) - FONT_CHARS)
    if bad:
        errs.append("Name can only use A-Z, 0-9, space and : ! - . > ? "
                    f"(not {' '.join(bad)}).")

    items = lv.get("items", {})
    for k in ITEMS:
        n = items.get(k, 0)
        if not isinstance(n, int) or not 0 <= n <= 99:
            errs.append(f"Item count for {k} must be 0-99.")

    maps = lv["maps"]
    if not 1 <= len(maps) <= MAX_MAPS:
        errs.append(f"A level has 1-{MAX_MAPS} maps (has {len(maps)}).")
        return errs
    multi = len(maps) > 1
    map_errs = []
    for k, m in enumerate(maps):
        prefix = f"Map {k + 1}: " if multi else ""
        map_errs += [prefix + e for e in validate_map(m)]
    errs += map_errs
    if map_errs:
        return errs

    starts = [(k, x, y) for k, m in enumerate(maps) for y, r in enumerate(m["map"])
              for x, c in enumerate(r) if c == "P"]
    if len(starts) != 1:
        errs.append(f"Level needs exactly one player start (has {len(starts)}).")
    elif starts[0][0] != 0:
        errs.append("The player start must be in the first map.")
    if not any("E" in r for m in maps for r in m["map"]):
        errs.append("Level needs at least one exit.")
    errs += validate_links(lv)
    return errs


def validate_all(levels):
    errs = []
    if not levels:
        errs.append("There must be at least one level.")
    if sum(len(upgrade(lv)["maps"]) for lv in levels) > 250:
        errs.append("Too many maps in total (max 250).")
    for n, lv in enumerate(levels):
        errs += [f"Level {n + 1} ({lv.get('name', '?')}): {e}" for e in validate_level(lv)]
    return errs


def c_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def generate_c(levels):
    """One LevelDef per map; a level's maps are consecutive, first map
    part 0, links as 1 + absolute index."""
    out = [
        "/* Generated by tools/gen_levels.py from levels/levels.json.",
        " * Don't edit by hand: use the level editor (make editor) or the JSON. */",
        '#include "levels.h"',
        "",
        "const LevelDef levels[LEVEL_COUNT] = {",
    ]
    base = 0
    for n, lv in enumerate(map(upgrade, levels)):
        notes = lv.get("notes", "").strip()
        links = link_table(lv)
        for k, m in enumerate(lv["maps"]):
            rows = m["map"]
            title = f"{n + 1}: {lv['name']}" + (f", map {k + 1}" if len(lv["maps"]) > 1 else "")
            out.append(f"    /* {title}" + ((" - " + notes).replace("*/", "* /")
                                            if notes and k == 0 else "") + " */")
            out.append("    {")
            out.append(f"        {c_string(lv['name'])},")
            out.append(f"        {len(rows[0])}, {len(rows)},")
            for j, r in enumerate(rows):
                out.append(f"            {c_string(r)}" + ("," if j == len(rows) - 1 else ""))
            guards = m.get("guards", [])
            out.append(f"        {len(guards)},")
            out.append("        {")
            for g in guards:
                pts = ", ".join(f"{{{x}, {y}}}" for x, y in g["route"])
                att = c_string(g["attitude"]) if g.get("attitude") else "0"
                out.append(f"            {{ DIR_{g['facing'].upper()}, {len(g['route'])}, "
                           f"{{ {pts} }}, DISGUISE_{g.get('uniform', 'none').upper()}, {att} }},")
            out.append("        },")
            items = lv.get("items", {}) if k == 0 else {}
            out.append("        { " + ", ".join(str(items.get(i, 0)) for i in ITEMS) + " },")
            link = [links.get((k, d)) for d in DIRS]
            out.append(f"        {k},   /* map {k + 1} of {len(lv['maps'])} */")
            out.append("        { " + ", ".join(str(base + l[0] + 1) if l else "0" for l in link)
                       + " },   /* up right down left */")
            out.append("        { " + ", ".join(str(l[1]) if l else "0" for l in link) + " },")
            out.append("    },")
        base += len(lv["maps"])
    out.append("};")
    out.append("")
    out += generate_art(levels)
    return "\n".join(out)


def generate_art(levels):
    """Tileset tiles used anywhere (as 2bpp data) and each map's MapArt."""
    maps = [m for lv in map(upgrade, levels) for m in lv["maps"]]
    used = sorted({v for m in maps for v in art_usage(m)})
    index = {v: i for i, v in enumerate(used)}
    tiles = tileset()["tiles"]
    out = ["/* Tileset tiles used by any map (16 bytes each), from tilesets/. */",
           "const uint8_t art_tile_data[] = {"]
    for v in used:
        out.append(f"    /* tileset tile {v} */ " + ", ".join(f"0x{b:02X}" for b in ts.to_2bpp(tiles[v])) + ",")
    if not used:
        out.append("    0")
    out.append("};")
    out.append("")
    refs = []
    for k, m in enumerate(maps):
        usage = art_usage(m)
        if not usage:
            refs.append("0")
            continue
        grid = ts.parse_art(m["art"])
        local = sorted(usage)                      # tileset tiles this map uses
        lidx = {v: i for i, v in enumerate(local)}
        data, rows = ts.rle_rows([[0xFF if v is None else lidx[v] for v in row] for row in grid])
        data += [0, 0]                             # end marker
        name = f"art{k}"
        out.append(f"static const uint8_t {name}_tiles[] = {{ {', '.join(str(index[v]) for v in local)} }};")
        out.append(f"static const uint8_t {name}_kinds[] = {{ "
                   f"{', '.join(str(ts.slots_for(usage[v])) for v in local)} }};")
        out.append(f"static const uint8_t {name}_rle[] = {{ {', '.join(str(b) for b in data)} }};")
        out.append(f"static const uint16_t {name}_rows[] = {{ {', '.join(str(r) for r in rows)} }};")
        out.append(f"static const MapArt {name} = {{ {len(local)}, {name}_tiles, {name}_kinds, "
                   f"{name}_rle, {name}_rows }};")
        refs.append(f"&{name}")
    out.append("")
    out.append("/* Art for each entry of levels[] (0 = built-in graphics only). */")
    out.append("const MapArt *const level_art[LEVEL_COUNT] = { " + ", ".join(refs) + " };")
    out.append("")
    return out


def generate_h(levels):
    count = sum(len(upgrade(lv)["maps"]) for lv in levels)
    return "\n".join([
        "/* Generated by tools/gen_levels.py - do not edit by hand. */",
        "#ifndef LEVELS_H",
        "#define LEVELS_H",
        "",
        '#include "game.h"',
        '#include "art.h"',
        "",
        "/* One entry per map; a level's maps are consecutive (part 0 first). */",
        f"#define LEVEL_COUNT {count}",
        "extern const LevelDef levels[LEVEL_COUNT];",
        "extern const uint8_t art_tile_data[];          /* 16 bytes per tile */",
        "extern const MapArt *const level_art[LEVEL_COUNT];",
        "",
        "#endif",
        "",
    ])


def dumps(levels):
    """JSON with one map row per line, so diffs and hand edits stay readable."""
    parts = []
    for lv in map(upgrade, levels):
        lines = ["  {"]
        lines.append(f'    "name": {json.dumps(lv["name"])},')
        lines.append(f'    "notes": {json.dumps(lv.get("notes", ""))},')
        lines.append(f'    "items": {json.dumps({k: lv.get("items", {}).get(k, 0) for k in ITEMS})},')
        lines.append('    "maps": [')
        ms = []
        for m in lv["maps"]:
            ml = ["      {", '        "map": [']
            ml.append(",\n".join(f"          {json.dumps(r)}" for r in m["map"]))
            ml.append("        ],")
            if m.get("art"):
                ml.append('        "art": [')
                ml.append(",\n".join(f"          {json.dumps(r)}" for r in m["art"]))
                ml.append("        ],")
            gl = []
            for g in m.get("guards", []):
                gl.append("          " + json.dumps({
                    "facing": g["facing"], "route": g["route"],
                    "uniform": g.get("uniform", "none"), "attitude": g.get("attitude")}))
            ml.append('        "guards": [' + ("\n" + ",\n".join(gl) + "\n        ]" if gl else "]"))
            ml.append("      }")
            ms.append("\n".join(ml))
        lines.append(",\n".join(ms))
        lines.append("    ],")
        ll = ["      " + json.dumps({"from": ln["from"], "edge": ln["edge"], "to": ln["to"],
                                      "offset": ln.get("offset", 0)})
              for ln in lv.get("links", [])]
        lines.append('    "links": [' + ("\n" + ",\n".join(ll) + "\n    ]" if ll else "]"))
        lines.append("  }")
        parts.append("\n".join(lines))
    return "[\n" + ",\n".join(parts) + "\n]\n"


def load(path=JSON_PATH):
    return [upgrade(lv) for lv in json.loads(Path(path).read_text())]


def save(levels, path=JSON_PATH):
    """Validate, then write the JSON and the generated C. Returns errors."""
    errs = validate_all(levels)
    if errs:
        return errs
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_text(dumps(levels))
    write_c(levels)
    return []


def write_c(levels):
    (ROOT / "src" / "levels.c").write_text(generate_c(levels))
    (ROOT / "src" / "levels.h").write_text(generate_h(levels))


def main():
    levels = load()
    errs = validate_all(levels)
    if errs:
        print("\n".join(errs), file=sys.stderr)
        sys.exit(1)
    write_c(levels)
    print(f"wrote {len(levels)} levels to src/levels.c")


if __name__ == "__main__":
    main()
