#!/usr/bin/env python3
"""Level data: levels/levels.json -> src/levels.c and src/levels.h.

    python3 tools/gen_levels.py          (or: make levels)

levels/levels.json is the source of truth; edit it with the level editor
(make editor) or by hand. The generated C files are committed so the ROM
builds without Python, but don't edit them directly. There are three, one
per ROM bank (src/far.h): levels.c (the level table, in bank 1 with the
code that reads it), levels_maps.c (map cells) and levels_art.c (painted
art), which are only mapped in while being copied out.

JSON format, one object per level:

    {
      "name":  "FIRST STEPS",              max 12 chars, HUD font only
      "notes": "Shown as a comment in levels.c",
      "items": {"stone": 0, "bottle": 0},  what the player starts with
      "transition": "fade",                how the screen dips to black when
                                           the level is finished: one of
                                           TRANSITIONS below
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
        "guards": [{                       the map's characters: guards,
                                           dogs, ... (at most 4)
          "facing":   "right",             up/right/down/left
          "route":    [[1, 5], [18, 5]],   patrol loop, starts at the first
                                           point; one point = stands still
          "uniform":  "none",              none/guard/officer (from body)
          "sees":     true,                optional: false = no cone at all
                                           (a dog): never catches you or
                                           notices bodies
          "enforcer": ["officer"],         optional: outfits it sees
                                           through (none/guard/officer):
                                           someone wearing one trespasses
                                           to it even where that's legal
          "look":     "guard",             optional: which sprites draw it
                                           (a name in tilesets/
                                           characters.json); default guard,
                                           or officer in an officer uniform
          "takedown": true,                optional: false = can't be taken
                                           down (always so if it talks)
          "verb":     "PET",               optional: talking to it with B,
          "script":   [ ... ]              HUD prompt B:PET (max 8 chars),
                                           and what it does: see below
        }],
        "access": {                        optional: where each outfit may
          "none":    ["TTLL...", ...],     be. Per outfit, a letter per
          "guard":   "L",                  cell (rows like the map): L
          "officer": "L"                   legal, T trespassing, H hostile;
        },                                 or one letter for the whole map.
                                           Guards enforce it: see ACCESS_*
                                           in src/game.h. Default: T for
                                           none (your own suit), L for the
                                           uniforms
        "switches": [{                     optional: cells that change when
          "x": 16, "y": 13,                a flag is set
          "flag": "dog",
          "on": "E",                       the map character it becomes
                                           (until then it is what the map
                                           says)
          "on_art": 52                     its tileset tile once set, or
                                           null for the built-in graphic
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

A script is a list of steps, run top to bottom when the player talks:

    {"say": "You pet the dog."}            a message box (upper-cased; up
                                           to 3 lines of 18 characters,
                                           wrapped for you)
    {"set": "dog"}                         set a flag (a name; up to 8 per
    {"clear": "dog"}                       level, shared by its maps)
    {"if": "dog", "then": [...],           steps for when the flag is set,
                  "else": [...]}           and (optional) for when it isn't

Flags start clear. Switch cells follow them at once.
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
# Level transitions, in TRANS_* order (src/trans.h); tests check they match.
TRANSITIONS = ["cut", "fade", "wipe", "curtain", "iris", "dissolve", "diagonal",
               "blinds", "interlace"]
DEFAULT_TRANSITION = "fade"
MAX_SWITCHES, MAX_FLAGS = 8, 8
VERB_MAX = 8
TEXT_COLS, TEXT_ROWS = 18, 3          # the message box (src/main.c)
# Access: per outfit, what a cell is for someone wearing it (ACCESS_* in
# src/game.h, in that order), and what a map without a layer says.
ACCESS = "LTH"
ACCESS_NAMES = {"L": "legal", "T": "trespassing", "H": "hostile"}
ACCESS_DEFAULT = {"none": "T", "guard": "L", "officer": "L"}
# script ops, as in src/game.h
OP_END, OP_SAY, OP_SET, OP_CLEAR, OP_IF, OP_JUMP = range(6)
FONT_CHARS = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 :!-.>?")


def art_slots_available():
    """Background tiles left for painted art: 256 minus the built-in ones
    (src/tiles.h)."""
    m = re.search(r"#define BKG_TILE_COUNT (\d+)", (ROOT / "src" / "tiles.h").read_text())
    return 256 - int(m.group(1))


_tileset = None
_looks = None


def tileset():
    global _tileset
    if _tileset is None:
        _tileset = ts.load()
    return _tileset


def looks():
    """The character looks sheet (tilesets/characters.json)."""
    global _looks
    if _looks is None:
        _looks = ts.load_looks()
    return _looks


def forget_tilesets():
    """Re-read the tileset and looks next time (after the editor changed
    them)."""
    global _tileset, _looks
    _tileset = _looks = None


def sprite_slots_available():
    """Sprite tiles left for character looks: 256 minus the built-in ones."""
    m = re.search(r"#define SPRITE_TILE_COUNT (\d+)", (ROOT / "src" / "tiles.h").read_text())
    return 256 - int(m.group(1))


def guard_look(g):
    """A character's look name: its own, or by its uniform."""
    if g.get("look"):
        return g["look"]
    names = looks()["looks"]
    want = "officer" if g.get("uniform") == "officer" else "guard"
    return want if want in names else names[0]


def can_talk(g):
    return bool(g.get("verb")) or bool(g.get("script"))

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


# CELL_* name (src/game.h) for each map character.
CELL_OF = {".": "FLOOR", "P": "FLOOR", "#": "WALL", "E": "EXIT", "D": "DOOR",
           "W": "WINDOW", "R": "RAILING", "S": "SECURITY", "C": "CURTAIN",
           "M": "MESH", "V": "VENT"}


def upgrade(lv):
    """Old one-map levels had map/guards at the top level; old maps had
    "npcs" next to their guards (now characters like any other)."""
    if "maps" not in lv:
        lv = dict(lv)
        lv["maps"] = [{"map": lv.pop("map", []), "guards": lv.pop("guards", [])}]
        lv.setdefault("links", [])
    for m in lv["maps"]:
        for n in m.pop("npcs", []):
            m.setdefault("guards", []).append({
                "facing": "down", "route": [[n.get("x"), n.get("y")]], "uniform": "none",
                "sees": False, "takedown": False,
                "verb": n.get("verb", "TALK"), "script": n.get("script", [])})
        for g in m.get("guards", []):
            upgrade_attitude(g)
    lv.setdefault("links", [])
    lv.setdefault("items", {})
    lv.setdefault("transition", DEFAULT_TRANSITION)
    return lv


def upgrade_attitude(g):
    """Guards used to carry an attitude to each outfit ("HBE": hostile,
    enforcer, blind, I = ignores). Now the map says who may be where and
    guards enforce it, so only two things survive: a character that
    ignored everyone doesn't watch, and a guard that wasn't fooled by a
    uniform (E or H) is an enforcer for it. Attitudes to your own suit are
    dropped: it trespasses wherever the map doesn't say otherwise."""
    if "attitude" not in g:
        return
    att = g.pop("attitude")
    if not isinstance(att, str) or len(att) != len(UNIFORMS):
        return
    if set(att) == {"I"}:
        g.setdefault("sees", False)
    else:
        seen = [u for u, a in zip(UNIFORMS, att) if a in "EH" and ACCESS_DEFAULT[u] == "L"]
        if seen:
            g.setdefault("enforcer", seen)


def access_rows(m, outfit):
    """One outfit's access layer as rows of L/T/H, the size of the map."""
    layer = (m.get("access") or {}).get(outfit, ACCESS_DEFAULT[outfit])
    if isinstance(layer, str):
        return [layer * len(r) for r in m["map"]]
    return layer


def access_bytes(m):
    """The access layer as the rules read it: a byte per cell, two bits per
    outfit (ACCESS_* in src/game.h)."""
    layers = [access_rows(m, u) for u in UNIFORMS]
    return [sum(ACCESS.index(layers[d][y][x]) << (2 * d) for d in range(len(UNIFORMS)))
            for y in range(len(m["map"])) for x in range(len(m["map"][0]))]


def access_is_default(m):
    return all(access_rows(m, u) == [ACCESS_DEFAULT[u] * len(r) for r in m["map"]]
               for u in UNIFORMS)


def access_rle(m):
    """access_bytes as (run, byte) pairs, runs of up to 255."""
    out = []
    for b in access_bytes(m):
        if out and out[-1] == b and out[-2] < 255:
            out[-2] += 1
        else:
            out += [1, b]
    return out


def validate_access(m):
    acc = m.get("access")
    if acc is None:
        return []
    if not isinstance(acc, dict) or set(acc) - set(UNIFORMS):
        return [f"Access must give a layer for some of: {', '.join(UNIFORMS)}."]
    rows = m["map"]
    for u, layer in acc.items():
        if isinstance(layer, str):
            ok = len(layer) == 1 and layer in ACCESS
        else:
            ok = (isinstance(layer, list) and len(layer) == len(rows) and
                  all(isinstance(r, str) and len(r) == len(rows[0]) and not set(r) - set(ACCESS)
                      for r in layer))
        if not ok:
            return [f"The access layer for {u} must be one of L, T, H, or rows of them "
                    "the size of the map."]
    return []


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
        errs.append(f"At most {MAX_GUARDS} characters (has {len(guards)}).")
    seen_starts = {}
    for i, g in enumerate(guards):
        tag = f"Character {i + 1}"
        route = g.get("route", [])
        if not 1 <= len(route) <= MAX_WAYPOINTS:
            errs.append(f"{tag}: route needs 1-{MAX_WAYPOINTS} points (has {len(route)}).")
            continue
        if g.get("facing") not in DIRS:
            errs.append(f"{tag}: facing must be one of {', '.join(DIRS)}.")
        if g.get("uniform", "none") not in UNIFORMS:
            errs.append(f"{tag}: uniform must be one of {', '.join(UNIFORMS)}.")
        if not isinstance(g.get("sees", True), bool):
            errs.append(f"{tag}: sees must be true or false.")
        enf = g.get("enforcer", [])
        if not isinstance(enf, list) or set(enf) - set(UNIFORMS):
            errs.append(f"{tag}: enforcer must be a list of {', '.join(UNIFORMS)}.")
        if g.get("look") is not None and g["look"] not in looks()["looks"]:
            errs.append(f"{tag}: there is no look called \"{g['look']}\" "
                        f"(tilesets/characters.json has {', '.join(looks()['looks'])}).")
        if not isinstance(g.get("takedown", True), bool):
            errs.append(f"{tag}: takedown must be true or false.")
        if can_talk(g):
            verb = g.get("verb", "")
            if not isinstance(verb, str) or not 1 <= len(verb) <= VERB_MAX or set(verb) - FONT_CHARS:
                errs.append(f"{tag}: the prompt word needs 1-{VERB_MAX} characters of "
                            "A-Z, 0-9 and : ! - . > ? (e.g. TALK).")
            errs += validate_script(g.get("script", []), tag)
        for w, (x, y) in enumerate(route):
            what = guard_blocked(x, y)
            if what:
                errs.append(f"{tag}: point {w + 1} ({x},{y}) is on a {what}, "
                            "which guards can't stand on.")
        start = tuple(route[0])
        if start in seen_starts:
            errs.append(f"{tag} starts on the same tile as character {seen_starts[start] + 1}.")
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
    errs += validate_access(m)
    return errs


def art_usage(m):
    """{tileset tile: set of map chars it's painted on} for one map,
    counting what switch cells turn into."""
    used = {}
    if not m.get("art"):
        return used
    for row, arow in zip(m["map"], ts.parse_art(m["art"])):
        for ch, v in zip(row, arow):
            if v is not None:
                used.setdefault(v, set()).add(ch)
    for sw in m.get("switches", []):
        if isinstance(sw.get("on_art"), int) and sw.get("on") in TILES:
            used.setdefault(sw["on_art"], set()).add(sw["on"])
    return used


def cell_rows(m):
    """The map as the rules read it: a switch cell is the digit of its
    index in the map's switches."""
    rows = [list(r) for r in m["map"]]
    for k, sw in enumerate(m.get("switches", [])):
        rows[sw["y"]][sw["x"]] = str(k)
    return ["".join(r) for r in rows]


# ---- scripts

def wrap_text(text):
    """A message as the box shows it: upper case, wrapped to TEXT_COLS.
    Raises ValueError (with a plain message) if it can't be shown."""
    text = " ".join(str(text).upper().split())
    if not text:
        raise ValueError("the message is empty")
    bad = sorted(set(text) - FONT_CHARS)
    if bad:
        raise ValueError(f"the font has no {' '.join(bad)} (only A-Z, 0-9 and : ! - . > ?)")
    lines, line = [], ""
    for word in text.split(" "):
        if len(word) > TEXT_COLS:
            raise ValueError(f"\"{word}\" is longer than a line ({TEXT_COLS} characters)")
        if line and len(line) + 1 + len(word) > TEXT_COLS:
            lines.append(line)
            line = word
        else:
            line = (line + " " + word).strip()
    lines.append(line)
    if len(lines) > TEXT_ROWS:
        raise ValueError(f"it needs {len(lines)} lines and the box has {TEXT_ROWS}; "
                         "split it into two messages")
    return lines


def script_steps(script):
    """Every step of a script, including those inside if/else."""
    for st in script if isinstance(script, list) else []:
        if isinstance(st, dict):
            yield st
            if "if" in st:
                yield from script_steps(st.get("then", []))
                yield from script_steps(st.get("else", []))


def level_scripts(lv):
    return [g.get("script", []) for m in lv["maps"] for g in m.get("guards", []) if can_talk(g)]


def level_flags(lv):
    """The level's flag names, in the order they get their bits."""
    names = []
    for m in lv["maps"]:
        for sw in m.get("switches", []):
            names.append(sw.get("flag"))
    for sc in level_scripts(lv):
        for st in script_steps(sc):
            names += [st[k] for k in ("set", "clear", "if") if k in st]
    out = []
    for n in names:
        if isinstance(n, str) and n and n not in out:
            out.append(n)
    return out


def compile_script(script, flags, texts):
    """Script steps -> bytes (OP_* in src/game.h). texts collects the
    messages; a SAY refers to its index there."""
    def mask(name):
        return 1 << flags.index(name)

    def block(steps):
        out = []
        for st in steps:
            if "say" in st:
                text = "\n".join(wrap_text(st["say"]))
                if text not in texts:
                    texts.append(text)
                out += [OP_SAY, texts.index(text)]
            elif "set" in st:
                out += [OP_SET, mask(st["set"])]
            elif "clear" in st:
                out += [OP_CLEAR, mask(st["clear"])]
            elif "if" in st:
                then, other = block(st.get("then", [])), block(st.get("else", []))
                if other:
                    then += [OP_JUMP, len(other)]
                out += [OP_IF, mask(st["if"]), len(then)] + then + other
        return out

    return block(script) + [OP_END]


def validate_script(script, tag):
    errs = []
    if not isinstance(script, list):
        return [f"{tag}: the script must be a list of steps."]

    def block(steps, depth):
        if not isinstance(steps, list):
            errs.append(f"{tag}: the steps of an \"if\" must be a list.")
            return 0
        size = 0
        for st in steps:
            kinds = [k for k in ("say", "set", "clear", "if") if isinstance(st, dict) and k in st]
            if len(kinds) != 1:
                errs.append(f"{tag}: each script step is one of say, set, clear or if.")
                continue
            kind = kinds[0]
            if kind == "say":
                try:
                    wrap_text(st["say"])
                except ValueError as e:
                    errs.append(f"{tag}: can't say \"{st['say']}\": {e}.")
                size += 2
            elif not isinstance(st[kind], str) or not st[kind].strip():
                errs.append(f"{tag}: \"{kind}\" needs the name of a flag.")
            elif kind == "if":
                if depth >= 4:
                    errs.append(f"{tag}: \"if\" steps are nested too deep.")
                    continue
                inner = block(st.get("then", []), depth + 1) + 2 + block(st.get("else", []), depth + 1)
                if inner > 250:
                    errs.append(f"{tag}: the steps inside one \"if\" are too long.")
                size += 3 + inner
            else:
                size += 2
        return size

    block(script, 0)
    return errs


def route_cells(g):
    """Every cell a guard's patrol passes over (straight legs only)."""
    route = [tuple(p) for p in g.get("route", [])]
    cells = set(route[:1])
    if len(route) > 1:
        for w in range(len(route)):
            (ax, ay), (bx, by) = route[w], route[(w + 1) % len(route)]
            if ax != bx and ay != by:
                continue
            sx, sy = (bx > ax) - (bx < ax), (by > ay) - (by < ay)
            x, y = ax, ay
            while (x, y) != (bx, by):
                x, y = x + sx, y + sy
                cells.add((x, y))
    return cells


def validate_scripting(m, set_flags):
    """Problems with a map's switch cells. set_flags: the flags some
    script in the level sets. (Characters' scripts are checked with the
    rest of the character, in validate_map.)"""
    errs = []
    rows = m["map"]
    mw, mh = map_size(m)
    switches = m.get("switches", [])
    if len(switches) > MAX_SWITCHES:
        errs.append(f"At most {MAX_SWITCHES} switch cells in a map (has {len(switches)}).")
    routes = set()
    for g in m.get("guards", []):
        if 1 <= len(g.get("route", [])) <= MAX_WAYPOINTS:
            routes |= route_cells(g)
    taken = {}

    def place(what, d):
        """Check a thing's cell; returns (x, y) or None."""
        x, y = d.get("x"), d.get("y")
        if not (isinstance(x, int) and isinstance(y, int) and 0 <= x < mw and 0 <= y < mh):
            errs.append(f"{what} is off the map.")
            return None
        if rows[y][x] == "P":
            errs.append(f"{what} at ({x},{y}) is on the player's start.")
        if (x, y) in taken:
            errs.append(f"{what} at ({x},{y}) shares its cell with {taken[(x, y)]}.")
        taken[(x, y)] = what[0].lower() + what[1:]
        return x, y

    for i, sw in enumerate(switches[:MAX_SWITCHES]):
        tag = f"Switch cell {i + 1}"
        at = place(tag, sw)
        flag, on = sw.get("flag"), sw.get("on")
        if not isinstance(flag, str) or not flag.strip():
            errs.append(f"{tag}: needs the name of the flag that changes it.")
        elif flag not in set_flags:
            errs.append(f"{tag}: nothing sets the flag \"{flag}\", so it would never change. "
                        "Give a character a \"set\" step for it.")
        if on not in TILES or on == "P":
            errs.append(f"{tag}: what it turns into must be one of "
                        f"{' '.join(c for c in TILES if c != 'P')}.")
        art = sw.get("on_art")
        if art is not None and not (isinstance(art, int) and 0 <= art < len(tileset()["tiles"])):
            errs.append(f"{tag}: its painted tile isn't in the tileset.")
        if at and on in TILES and at in routes and (TILES[rows[at[1]][at[0]]][2] or TILES[on][2]):
            errs.append(f"{tag} at ({at[0]},{at[1]}) is on a guard's route, and would block it.")
    return errs


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
    good = []      # links that are well-formed enough to follow
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
        good.append(ln)
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
    table = link_table({"links": good})
    reach, todo = {0}, [0]
    while todo:
        m = todo.pop()
        for (src, _), (dst, _) in table.items():
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

    if lv["transition"] not in TRANSITIONS:
        errs.append(f"Transition must be one of {', '.join(TRANSITIONS)}.")

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

    # scripts and switch cells (the maps themselves are sound by now)
    set_flags = {st["set"] for sc in level_scripts(lv) for st in script_steps(sc)
                 if isinstance(st.get("set"), str)}
    script_errs = []
    for k, m in enumerate(maps):
        prefix = f"Map {k + 1}: " if multi else ""
        script_errs += [prefix + e for e in validate_scripting(m, set_flags)]
    flags = level_flags(lv)
    if len(flags) > MAX_FLAGS:
        script_errs.append(f"A level can use {MAX_FLAGS} flags (this one uses {len(flags)}: "
                           f"{', '.join(flags)}).")
    errs += script_errs
    if script_errs:
        return errs

    starts = [(k, x, y) for k, m in enumerate(maps) for y, r in enumerate(m["map"])
              for x, c in enumerate(r) if c == "P"]
    if len(starts) != 1:
        errs.append(f"Level needs exactly one player start (has {len(starts)}).")
    elif starts[0][0] != 0:
        errs.append("The player start must be in the first map.")
    if not any("E" in r for m in maps for r in m["map"]) and \
            not any(sw["on"] == "E" for m in maps for sw in m.get("switches", [])):
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
    need = len(looks()["looks"]) * ts.LOOK_FRAMES
    if need > sprite_slots_available():
        errs.append(f"The character looks need {need} sprite tiles; the Game Boy has room "
                    f"for {sprite_slots_available()} ({sprite_slots_available() // ts.LOOK_FRAMES} looks).")
    if not errs and len(compile_all(levels)[1]) > 255:
        errs.append("Too many different messages in total (max 255).")
    return errs


def compile_all(levels):
    """Every character's script as bytes, and the text they say:
    ({(map index in the table, character index): bytes}, [messages])."""
    scripts, texts, base = {}, [], 0
    for lv in map(upgrade, levels):
        flags = level_flags(lv)
        for k, m in enumerate(lv["maps"]):
            for i, g in enumerate(m.get("guards", [])):
                if can_talk(g):
                    scripts[(base + k, i)] = compile_script(g.get("script", []), flags, texts)
        base += len(lv["maps"])
    return scripts, texts


def c_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


GENERATED = ["/* Generated by tools/gen_levels.py from levels/levels.json.",
             " * Don't edit by hand: use the level editor (make editor) or the JSON. */"]


def bank_pragma(bank, why):
    """Put a generated file in a ROM bank (ignored by gcc). Numbers as in
    src/far.h."""
    return ["#ifdef __SDCC", f"#pragma bank {bank}     /* {why}: see far.h */", "#endif", ""]


def all_maps(levels):
    return [m for lv in map(upgrade, levels) for m in lv["maps"]]


def generate_maps(levels):
    """src/levels_maps.c: every map's cells, in ROM bank BANK_MAPS."""
    out = GENERATED + ['#include "levels.h"', ""] + bank_pragma(2, "BANK_MAPS")
    for k, m in enumerate(all_maps(levels)):
        rows = cell_rows(m)
        out.append(f"const char level_map{k}[] =")
        out += [f"    {c_string(r)}" + (";" if j == len(rows) - 1 else "") for j, r in enumerate(rows)]
        if not access_is_default(m):
            out.append("/* who may be where: (run, byte) pairs, ACCESS_* bits per cell */")
            out.append(f"const uint8_t level_access{k}[] = {{ "
                       f"{', '.join(map(str, access_rle(m)))} }};")
        out.append("")
    return "\n".join(out)


def generate_c(levels):
    """src/levels.c: one LevelDef per map; a level's maps are consecutive,
    first map part 0, links as 1 + absolute index."""
    out = GENERATED + ['#include "levels.h"', ""] + bank_pragma(1, "with main.c")
    count = len(all_maps(levels))
    out.append("/* Map cells: in levels_maps.c (ROM bank BANK_MAPS). */")
    out += [f"extern const char level_map{k}[];" for k in range(count)]
    out += [f"extern const uint8_t level_access{k}[];"
            for k, m in enumerate(all_maps(levels)) if not access_is_default(m)]
    scripts, texts = compile_all(levels)
    if scripts:
        out += ["", "/* Characters' scripts (OP_* in game.h). */"]
        out += [f"static const uint8_t script{k}_{i}[] = {{ {', '.join(map(str, code))} }};"
                for (k, i), code in scripts.items()]
    out += ["", "const LevelDef levels[LEVEL_COUNT] = {"]
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
            out.append(f"        level_map{base + k},")
            guards = m.get("guards", [])
            out.append(f"        {len(guards)},")
            out.append("        {")
            for i, g in enumerate(guards):
                pts = ", ".join(f"{{{x}, {y}}}" for x, y in g["route"])
                enf = sum(1 << UNIFORMS.index(u) for u in set(g.get("enforcer", [])))
                att = f"0x{enf:02X}" if enf else "0"
                talk = can_talk(g)
                flags_c = " | ".join(
                    (["CHAR_NO_TAKEDOWN"] if talk or g.get("takedown") is False else []) +
                    (["CHAR_NO_SIGHT"] if g.get("sees") is False else [])) or "0"
                verb = c_string(g["verb"]) if talk else "0"
                script = f"script{base + k}_{i}" if talk else "0"
                out.append(f"            {{ DIR_{g['facing'].upper()}, {len(g['route'])}, "
                           f"{{ {pts} }}, DISGUISE_{g.get('uniform', 'none').upper()}, {att}, "
                           f"{flags_c}, LOOK_{guard_look(g).upper()}, {verb}, {script} }},")
            out.append("        },")
            items = lv.get("items", {}) if k == 0 else {}
            out.append("        { " + ", ".join(str(items.get(i, 0)) for i in ITEMS) + " },")
            link = [links.get((k, d)) for d in DIRS]
            out.append(f"        {k},   /* map {k + 1} of {len(lv['maps'])} */")
            out.append("        { " + ", ".join(str(base + l[0] + 1) if l else "0" for l in link)
                       + " },   /* up right down left */")
            out.append("        { " + ", ".join(str(l[1]) if l else "0" for l in link) + " },")
            out.append(f"        TRANS_{lv['transition'].upper()},")
            flags = level_flags(lv)
            local = sorted(art_usage(m))          # as generate_art numbers them
            switches = m.get("switches", [])
            out.append(f"        {len(switches)},")
            out.append("        {")
            for sw in switches:
                art = local.index(sw["on_art"]) if sw.get("on_art") is not None and local else 255
                out.append(f"            {{ {sw['x']}, {sw['y']}, {1 << flags.index(sw['flag'])}, "
                           f"CELL_{CELL_OF[rows[sw['y']][sw['x']]]}, CELL_{CELL_OF[sw['on']]}, {art} }},"
                           f"   /* {sw['flag']} */")
            if not switches:
                out.append("            { 0, 0, 0, 0, 0, 0 },")
            out.append("        },")
            out.append(f"        {'0' if access_is_default(m) else f'level_access{base + k}'},"
                       "   /* access */")
            out.append("    },")
        base += len(lv["maps"])
    out.append("};")
    out.append("")
    out.append("/* What characters say (OP_SAY n shows level_text[n]); lines end in \\n. */")
    out.append("const char *const level_text[] = {")
    out += [f"    {c_string(t)}," for t in texts] or ["    0,"]
    out.append("};")
    out.append("")
    out += generate_art(levels)[0]
    out += generate_looks()
    return "\n".join(out)


def generate_looks():
    """Character looks (sprites, 16 bytes per tile): LOOK_FRAMES tiles per
    look, loaded into sprite VRAM after the built-in ones."""
    out = ["/* Character looks, from tilesets/characters.png: up right down left",
           " * subdued body. */", "const uint8_t look_tile_data[] = {"]
    for name, frames in zip(looks()["looks"], looks()["frames_of"]):
        out.append(f"    /* {name} */")
        for px in frames:
            out.append("    " + ", ".join(f"0x{b:02X}" for b in ts.to_2bpp(px)) + ",")
    out += ["};", ""]
    return out


def generate_art_data(levels):
    """src/levels_art.c: tileset tiles and run-length art, in ROM bank
    BANK_ART."""
    out = GENERATED + ['#include "levels.h"', ""] + bank_pragma(3, "BANK_ART")
    return "\n".join(out + generate_art(levels)[1])


def generate_art(levels):
    """Each map's painted art, as C lines for (levels.c, levels_art.c):
    the small MapArt records that code reads directly, and the bulky data
    (tileset tiles as 2bpp, run-length art) that goes in the art bank."""
    maps = all_maps(levels)
    used = sorted({v for m in maps for v in art_usage(m)})
    index = {v: i for i, v in enumerate(used)}
    tiles = tileset()["tiles"]
    far = ["/* Tileset tiles used by any map (16 bytes each), from tilesets/. */",
           "const uint8_t art_tile_data[] = {"]
    for v in used:
        far.append(f"    /* tileset tile {v} */ " + ", ".join(f"0x{b:02X}" for b in ts.to_2bpp(tiles[v])) + ",")
    if not used:
        far.append("    0")
    far.append("};")
    far.append("")
    out = ["/* Painted art. The run-length data is in levels_art.c (ROM bank BANK_ART). */"]
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
        far.append(f"const uint8_t {name}_rle[] = {{ {', '.join(str(b) for b in data)} }};")
        far.append(f"const uint16_t {name}_rows[] = {{ {', '.join(str(r) for r in rows)} }};")
        out.append(f"extern const uint8_t {name}_rle[];")
        out.append(f"extern const uint16_t {name}_rows[];")
        out.append(f"static const uint8_t {name}_tiles[] = {{ {', '.join(str(index[v]) for v in local)} }};")
        out.append(f"static const uint8_t {name}_kinds[] = {{ "
                   f"{', '.join(str(ts.slots_for(usage[v])) for v in local)} }};")
        out.append(f"static const MapArt {name} = {{ {len(local)}, {name}_tiles, {name}_kinds, "
                   f"{name}_rle, {name}_rows }};")
        refs.append(f"&{name}")
    out.append("")
    out.append("/* Art for each entry of levels[] (0 = built-in graphics only). */")
    out.append("const MapArt *const level_art[LEVEL_COUNT] = { " + ", ".join(refs) + " };")
    out.append("")
    far.append("")
    return out, far


def generate_h(levels):
    count = sum(len(upgrade(lv)["maps"]) for lv in levels)
    return "\n".join([
        "/* Generated by tools/gen_levels.py - do not edit by hand. */",
        "#ifndef LEVELS_H",
        "#define LEVELS_H",
        "",
        '#include "game.h"',
        '#include "art.h"',
        '#include "trans.h"',
        "",
        "/* One entry per map; a level's maps are consecutive (part 0 first). */",
        f"#define LEVEL_COUNT {count}",
        "extern const LevelDef levels[LEVEL_COUNT];",
        "/* 16 bytes per tile; in ROM bank BANK_ART (far.h): read with far_read */",
        "extern const uint8_t art_tile_data[];",
        "extern const MapArt *const level_art[LEVEL_COUNT];",
        "extern const char *const level_text[];        /* what characters say */",
        "",
        "/* Character looks (GuardDef.look), from tilesets/characters.json. */",
        f"#define LOOK_COUNT {len(looks()['looks'])}",
        f"#define LOOK_FRAMES {ts.LOOK_FRAMES}   /* up right down left subdued body */",
    ] + [f"#define LOOK_{n.upper()} {i}" for i, n in enumerate(looks()["looks"])] + [
        "extern const uint8_t look_tile_data[];         /* LOOK_COUNT * LOOK_FRAMES tiles */",
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
        lines.append(f'    "transition": {json.dumps(lv["transition"])},')
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
                d = {"facing": g["facing"], "route": g["route"],
                     "uniform": g.get("uniform", "none")}
                if g.get("sees") is False:
                    d["sees"] = False
                if g.get("enforcer"):
                    d["enforcer"] = [u for u in UNIFORMS if u in g["enforcer"]]
                if g.get("look"):
                    d["look"] = g["look"]
                if g.get("takedown") is False and not can_talk(g):
                    d["takedown"] = False
                if can_talk(g):
                    d["verb"], d["script"] = g.get("verb", ""), g.get("script", [])
                gl.append("          " + json.dumps(d))
            ml.append('        "guards": [' + ("\n" + ",\n".join(gl) + "\n        ]" if gl else "]"))
            if not access_is_default(m):           # a letter where a layer is all one thing
                acc = {}
                for u in UNIFORMS:
                    layer = access_rows(m, u)
                    flat = set("".join(layer))
                    if len(flat) == 1:
                        if flat != {ACCESS_DEFAULT[u]}:
                            acc[u] = flat.pop()
                    else:
                        acc[u] = layer
                ml[-1] += ","
                ml.append('        "access": {\n' + ",\n".join(
                    f'          "{u}": ' + (json.dumps(v) if isinstance(v, str) else
                                             "[\n" + ",\n".join(f"            {json.dumps(r)}" for r in v) +
                                             "\n          ]")
                    for u, v in acc.items()) + "\n        }")
            for key in ("switches",):              # only where there are any
                if m.get(key):
                    ml[-1] += ","
                    ml.append(f'        "{key}": [\n' +
                              ",\n".join("          " + json.dumps(d) for d in m[key]) +
                              "\n        ]")
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
    for name, text in generated_files(levels).items():
        (ROOT / "src" / name).write_text(text)


def generated_files(levels):
    """{file name in src/: contents} for everything this script writes."""
    return {"levels.c": generate_c(levels), "levels_maps.c": generate_maps(levels),
            "levels_art.c": generate_art_data(levels), "levels.h": generate_h(levels)}


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
