#!/usr/bin/env python3
"""Smoke test for the level editor's bridge to the game rules.

    make test-editor

Checks that the ctypes mirror matches src/game.h (game_lib raises on
import if not), that a level round-trips through the JSON generator, and
that playtest steps and the solver work through the shared library.
"""
import json
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "editor"))

import gen_levels   # noqa: E402
import game_lib as g   # noqa: E402

levels = gen_levels.load()
assert not gen_levels.validate_all(levels), gen_levels.validate_all(levels)

# generated C must match the committed files
assert gen_levels.generate_c(levels) == (ROOT / "src" / "levels.c").read_text(), \
    "src/levels.c is out of date: run make levels"
assert gen_levels.generate_h(levels) == (ROOT / "src" / "levels.h").read_text(), \
    "src/levels.h is out of date: run make levels"

def with_map(lv, rows, k=0):
    """Copy of a level with map k's rows replaced."""
    lv = json.loads(json.dumps(lv))
    lv["maps"][k]["map"] = rows
    lv["maps"][k].pop("art", None)
    return lv


first_rows = levels[0]["maps"][0]["map"]

# playtest through the real rules
lv = g.Level(levels[0])
w = g.new_game(lv)
v = g.view(lv, w)
assert v["player"] == [1, 1] and v["status"] == g.G["STATUS_PLAYING"]
assert g.step(lv, w, g.G["ACT_RIGHT"]) and w.part[0].px == 2
assert not g.step(lv, w, g.G["ACT_UP"]), "walked into a wall"
token = g.encode(w)
assert g.decode(token).part[0].px == 2

# solver gives the same answer as the C tests
r = g.solve(lv, g.G["SOLVE_USE_ALL"])
assert r["length"] == 20, r

# a two-screen-wide map works end to end
wide = with_map(levels[0], [row + "#" * 20 for row in first_rows])
assert not gen_levels.validate_level(wide), gen_levels.validate_level(wide)
lw = g.Level(wide)
vw = g.view(lw, g.new_game(lw))
assert len(vw["tiles"][0]) == 40 and len(vw["tiles"]) == 16
assert g.solve(lw, g.G["SOLVE_USE_ALL"])["length"] == 20
too_big = with_map(levels[0], [row * 3 for row in first_rows] * 3)
assert any("tiles" in e for e in gen_levels.validate_level(too_big))
partial = with_map(levels[0], [row + "#" * 5 for row in first_rows])
assert any("whole screens" in e for e in gen_levels.validate_level(partial))

# a level of two linked maps: walk right off map 1 into map 2
def room(start=False, exit=False, open_right=(), open_left=()):
    rows = []
    for y in range(16):
        r = ""
        for x in range(20):
            edge = x in (0, 19) or y in (0, 15)
            opening = (x == 19 and y in open_right) or (x == 0 and y in open_left)
            r += "." if opening or not edge else "#"
        rows.append(r)
    if start:
        rows[2] = rows[2][:2] + "P" + rows[2][3:]
    if exit:
        rows[12] = rows[12][:15] + "E" + rows[12][16:]
    return rows

two = {"name": "TWO", "notes": "", "items": {"stone": 0, "bottle": 0},
       "maps": [{"map": room(start=True, open_right=(5, 6)), "guards": []},
                {"map": room(exit=True, open_left=(3, 4)), "guards": []}],
       "links": [{"from": 0, "edge": "right", "to": 1, "offset": 2}]}
assert not gen_levels.validate_level(two), gen_levels.validate_level(two)
lt = g.Level(two)
assert lt.table[0].link[1] == 2 and lt.table[1].link[3] == 1, "links both ways"
assert lt.table[1].link_offset[3] == -2
wt = g.new_game(lt)
wt.part[0].px, wt.part[0].py = 19, 6
assert g.step(lt, wt, g.G["ACT_RIGHT"]) and wt.cur == 1
assert g.view(lt, wt)["player"] == [0, 4] and g.view(lt, wt)["map"] == 1
assert g.solve(lt, 0)["length"] == 20 + 1 + 24
# generated C links map 1 <-> map 2 by absolute index
c = gen_levels.generate_c(levels + [two])
n = sum(len(x["maps"]) for x in levels)
assert f"{{ 0, {n + 2}, 0, 0 }}" in c and f"{{ 0, 0, 0, {n + 1} }}" in c
# validation: openings must line up, and every map must be reachable
bad_offset = json.loads(json.dumps(two))
bad_offset["links"][0]["offset"] = 0
assert any("leads into a wall" in e for e in gen_levels.validate_level(bad_offset))
orphan = json.loads(json.dumps(two))
orphan["links"] = []
assert any("isn't linked" in e for e in gen_levels.validate_level(orphan))
start2 = json.loads(json.dumps(two))
start2["maps"][1]["map"] = room(exit=True, open_left=(3, 4))
start2["maps"][1]["map"][5] = "#.P" + start2["maps"][1]["map"][5][3:]
start2["maps"][0]["map"] = room(open_right=(5, 6))
assert any("first map" in e for e in gen_levels.validate_level(start2))

# the tile table in gen_levels.py matches cell_blocks[] in game.c
for ch, (name, b_player, b_guard, b_sight, b_throw) in gen_levels.TILES.items():
    blocks = g.CELL_BLOCKS[g.G["CELL_" + g.CELL_CHARS[ch]]]
    want = (b_player * g.G["BLOCK_PLAYER"] | b_guard * g.G["BLOCK_GUARD"] |
            b_sight * g.G["BLOCK_SIGHT"] | b_throw * g.G["BLOCK_THROW"])
    assert blocks == want, f"tile {ch} ({name}): C says {blocks}, Python says {want}"

# painted art: the Game Boy's variant maker (src/art.c) and run-length
# decoder agree with tools/tileset.py, for every tile in the tileset
import ctypes as C   # noqa: E402
import tileset as ts   # noqa: E402
art_variant = g.lib.art_variant
art_variant.argtypes = [C.POINTER(C.c_uint8), C.c_uint8, C.POINTER(C.c_uint8)]
tset = ts.load()
for n, px in enumerate(tset["tiles"]):
    src = (C.c_uint8 * 16)(*ts.to_2bpp(px))
    for kind in range(5):
        dst = (C.c_uint8 * 16)()
        art_variant(src, kind, dst)
        assert list(dst) == ts.to_2bpp(ts.variant(px, kind)), f"tile {n} variant {kind}"


class MapArt(C.Structure):
    _fields_ = [("count", C.c_uint8), ("tiles", C.POINTER(C.c_uint8)),
                ("kinds", C.POINTER(C.c_uint8)), ("rle", C.POINTER(C.c_uint8)),
                ("rows", C.POINTER(C.c_uint16))]


g.lib.art_row.argtypes = [C.POINTER(MapArt), C.c_uint8, C.c_uint8, C.c_uint8, C.POINTER(C.c_uint8)]
grid = [[None, None, 3, 3, 3, 0, 0, None, 7, 7] * 4, [1] * 40]
data, rows = ts.rle_rows([[0xFF if v is None else v for v in r] for r in grid])
data += [0, 0]
rle = (C.c_uint8 * len(data))(*data)
offs = (C.c_uint16 * len(rows))(*rows)
ma = MapArt(0, None, None, rle, offs)
for y, row in enumerate(grid):
    for x0, w in [(0, 20), (3, 20), (17, 20), (20, 20), (9, 1)]:
        out = (C.c_uint8 * 20)()
        g.lib.art_row(C.byref(ma), y, x0, w, out)
        want = [0xFF if v is None else v for v in row[x0:x0 + w]]
        assert list(out)[:w] == want, (y, x0, list(out)[:w], want)

# art layers: level 1 is painted; generated data and validation
lv1 = levels[0]["maps"][0]
assert lv1.get("art") and "level_art[LEVEL_COUNT] = { &art0" in gen_levels.generate_c(levels)
bad_art = json.loads(json.dumps(levels[0]))
bad_art["maps"][0]["art"][0] = bad_art["maps"][0]["art"][0][:-2]
assert any("doesn't match" in e for e in gen_levels.validate_level(bad_art))
greedy = json.loads(json.dumps(levels[0]))     # 100 different tiles on floor
greedy["maps"][0]["map"] = ["#" * 20] + ["#" + "." * 18 + "#"] * 14 + ["#" * 20]
greedy["maps"][0]["map"][1] = "#P" + "." * 17 + "#"
greedy["maps"][0]["map"][2] = "#E" + "." * 17 + "#"
greedy["maps"][0]["guards"] = []
greedy["maps"][0]["art"] = ts.format_art(
    [[(y * 20 + x) % 100 for x in range(20)] for y in range(16)])
assert any("tile slots" in e for e in gen_levels.validate_level(greedy))

# tile painter: saving a tile round-trips exactly through the PNG, leaves
# other tiles alone, and adding a row keeps existing numbers (on a copy)
import random   # noqa: E402
import shutil   # noqa: E402
import tempfile   # noqa: E402
with tempfile.TemporaryDirectory() as tmp:
    for f in ("dungeon.json", "dungeon.png"):
        shutil.copy(ROOT / "tilesets" / f, tmp)
    copy = Path(tmp) / "dungeon.json"
    before = ts.load(copy)["tiles"]
    pattern = [random.randrange(4) for _ in range(64)]
    ts.set_tile(7, pattern, copy)
    after = ts.load(copy)["tiles"]
    assert after[7] == pattern and all(after[i] == before[i] for i in range(len(before)) if i != 7)
    ts.add_row(copy)
    grown = ts.load(copy)["tiles"]
    assert len(grown) == len(before) + 16 and grown[7] == pattern and not any(map(any, grown[-16:]))

# auto-paint: the wall rules, checked against a hand-painted level 1
import autopaint   # noqa: E402
ap = autopaint.autopaint(first_rows, tset["autopaint"])
expect = {(0, 0): 0, (1, 0): 66, (2, 0): 1, (7, 0): 3,        # top wall, corners
          (0, 1): 16, (7, 1): 19,                               # sides
          (1, 1): 87, (3, 4): 36,                               # floor, doorway
          (0, 4): 0, (19, 4): 3, (0, 7): 32, (19, 7): 35,       # hallway corners
          (9, 7): 0, (15, 7): 3, (12, 7): 36,                   # room below the hallway
          (1, 7): 1, (13, 9): 52, (9, 11): 32, (15, 11): 35,    # bottom edges, exit
          (10, 13): None}                                       # the void
for (x, y), want in expect.items():
    assert ap[y][x] == want, f"auto-paint ({x},{y}): {ap[y][x]} != {want}"

# auto-tile learns from painted examples, and repaints only around edits
cfg = tset["autopaint"]
room = ["#" * 20] + ["#" + "." * 18 + "#"] * 14 + ["#" * 20]
taught = autopaint.autopaint(room, cfg)
taught[0][0] = 7                                   # "use tile 7 for this corner"
lesson = [{"maps": [{"map": room, "art": ts.format_art(taught)}]}]
learned = autopaint.learn(lesson, cfg)
other = ["#" * 20] * 3 + ["###" + "." * 14 + "###"] * 10 + ["#" * 20] * 3
assert autopaint.autopaint(other, cfg)[2][2] == cfg["top_left"]       # the rule
assert autopaint.autopaint(other, cfg, learned)[2][2] == 7            # the lesson
assert autopaint.autopaint(other, cfg, learned)[3][2] == cfg["left"]  # untaught: rule
# a shape taught by hand beats what most painted cells say
shape = autopaint.taught_key(autopaint.pattern(other, 2, 3))
assert autopaint.learn(lesson, cfg)[autopaint.pattern(other, 2, 3)] == cfg["left"]
taught_cfg = dict(cfg, taught={shape: 5})
assert autopaint.autopaint(other, cfg, autopaint.learn(lesson, taught_cfg))[3][2] == 5
base = ts.format_art(autopaint.autopaint(room, cfg))
cut = [r[:] for r in room]
cut[8] = cut[8][:10] + "#" + cut[8][11:]           # a pillar in the middle
after = ts.parse_art(autopaint.autotile(cut, base, [(10, 8)], cfg))
before = ts.parse_art(base)
assert after[8][10] is not None and after[8][10] != before[8][10], "pillar drawn"
changed = {(x, y) for y in range(16) for x in range(20) if after[y][x] != before[y][x]}
assert changed and all(abs(x - 10) <= 1 and abs(y - 8) <= 1 for x, y in changed), changed

# a malformed link is reported, not a crash
bad_edge = json.loads(json.dumps(two))
bad_edge["links"][0]["edge"] = "sideways"
assert any("edge must be" in e for e in gen_levels.validate_level(bad_edge))

# the editor's Solve says which mechanics a level needs (level 5: CLEANUP)
os.environ["EDITOR_NO_BROWSER"] = "1"
import server   # noqa: E402
needs = server.analyze(g.Level(levels[4]))["needs"]
assert needs.get("takedown") == "yes" and needs.get("drag") == "yes", needs

# validation catches common mistakes
bad = with_map(levels[0], [row.replace("P", ".") for row in first_rows])
assert any("player start" in e for e in gen_levels.validate_level(bad))
print("editor bridge OK")
