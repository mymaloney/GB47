#!/usr/bin/env python3
"""Smoke test for the level editor's bridge to the game rules.

    make test-editor

Checks that the ctypes mirror matches src/game.h (game_lib raises on
import if not), that a level round-trips through the JSON generator, and
that playtest steps and the solver work through the shared library.
"""
import json
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

# validation catches common mistakes
bad = with_map(levels[0], [row.replace("P", ".") for row in first_rows])
assert any("player start" in e for e in gen_levels.validate_level(bad))
print("editor bridge OK")
