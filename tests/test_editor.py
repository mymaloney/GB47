#!/usr/bin/env python3
"""Smoke test for the level editor's bridge to the game rules.

    make test-editor

Checks that the ctypes mirror matches src/game.h (game_lib raises on
import if not), that a level round-trips through the JSON generator, and
that playtest steps and the solver work through the shared library.
"""
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

# playtest through the real rules
lv = g.Level(levels[0])
s = g.new_game(lv)
v = g.view(lv, s)
assert v["player"] == [1, 1] and v["status"] == g.G["STATUS_PLAYING"]
assert g.step(lv, s, g.G["ACT_RIGHT"]) and s.px == 2
assert not g.step(lv, s, g.G["ACT_UP"]), "walked into a wall"
token = g.encode(s)
assert g.decode(token).px == 2

# solver gives the same answer as the C tests
r = g.solve(lv, g.G["SOLVE_USE_ALL"])
assert r["length"] == 20, r

# a two-screen-wide level works end to end
wide = dict(levels[0], map=[row + "#" * 20 for row in levels[0]["map"]])
assert not gen_levels.validate_level(wide), gen_levels.validate_level(wide)
lw = g.Level(wide)
vw = g.view(lw, g.new_game(lw))
assert len(vw["tiles"][0]) == 40 and len(vw["tiles"]) == 16
assert g.solve(lw, g.G["SOLVE_USE_ALL"])["length"] == 20
too_big = dict(levels[0], map=[row * 3 for row in levels[0]["map"]] * 3)
assert any("tiles" in e for e in gen_levels.validate_level(too_big))
partial = dict(levels[0], map=[row + "#" * 5 for row in levels[0]["map"]])
assert any("whole screens" in e for e in gen_levels.validate_level(partial))

# the tile table in gen_levels.py matches cell_blocks[] in game.c
for ch, (name, b_player, b_guard, b_sight, b_throw) in gen_levels.TILES.items():
    blocks = g.CELL_BLOCKS[g.G["CELL_" + g.CELL_CHARS[ch]]]
    want = (b_player * g.G["BLOCK_PLAYER"] | b_guard * g.G["BLOCK_GUARD"] |
            b_sight * g.G["BLOCK_SIGHT"] | b_throw * g.G["BLOCK_THROW"])
    assert blocks == want, f"tile {ch} ({name}): C says {blocks}, Python says {want}"

# validation catches common mistakes
bad = dict(levels[0], map=[row.replace("P", ".") for row in levels[0]["map"]])
assert any("player start" in e for e in gen_levels.validate_level(bad))
print("editor bridge OK")
