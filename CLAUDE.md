# GB47: notes for coding agents

A turn-based stealth puzzle game for the original Game Boy (GBDK-2020 4.3.0,
SDCC, MBC5), plus a browser level editor. Read `README.md` for the player
and editor view. **The current task is in `docs/dialogue-plan.md`.**

## Commands

```bash
make test          # rules unit tests + solver proves every level (gcc only)
make test-editor   # editor bridge, auto-paint, and levels.c up to date
make               # the ROM (needs GBDK at ~/gbdk or GBDK_HOME=...)
make usage         # ROM bank / RAM usage (romusage)
make editor        # the editor at http://127.0.0.1:8047
```

Run `make test` and `make test-editor` before every commit; CI
(`.github/workflows/build.yml`) runs both and builds the ROM. Download
GBDK 4.3.0 from its GitHub releases if `make` can't find `lcc`.

## How the code is split

- `src/game.c/h`: **all rules**, pure C, no hardware. It's compiled
  three ways: into the ROM, natively for `tests/test_game.c`, and as
  `build/libgame.so`, which the editor loads through ctypes. Anything
  that changes what happens in a turn goes here, so undo, the solver and
  the editor's playtest all get it for free.
- `src/world.c/h`: levels of up to 4 linked maps, crossings, and undo. Undo
  entries are `{current map, GameState}` and are taken before each turn.
- `src/main.c`: Game Boy only (input, drawing, HUD, windows). It only
  *reads* `GameState`, except through `world_step`/`world_throw`.
- `src/levels.c/h`: **generated** from `levels/levels.json` by
  `tools/gen_levels.py`. Never edit it by hand. The JSON format is
  documented at the top of `gen_levels.py`.
- `src/trans.c/h`: level transition patterns (pure C, also in
  `libgame.so` for the editor's preview). `main.c` plays them; a level's
  choice is `LevelDef.transition`, `"transition"` in the JSON. Adding one:
  see the top of `trans.h`.
- `src/art.c`, `tools/tileset.py`, `tilesets/`: the painted art layer.
  `tools/autopaint.py` handles auto-paint and auto-tile.
- `tools/solver.c`: A* solver used by the tests and the editor. It
  enumerates actions itself, so **every new action needs adding to it**.
- `tools/editor/`:
  - `server.py`: the API.
  - `game_lib.py`: the ctypes mirror of `GameState`/`LevelDef`. **Update
    it whenever those structs change**; it checks the sizes against
    `editor_info()` in `tools/editor_lib.c` and refuses to start on a
    mismatch.
  - `editor.js`/`index.html`/`editor.css`: the UI.
- `tests/test_game.c`:
  - The `required[]` / `required_tiles[]` tables assert that a level
    *needs* the mechanic it teaches.
  - The solver prints every level's shortest solution.

## SDCC / Game Boy gotchas

- No struct initialisation from another struct
  (`GameState b = state;` fails with "error 69: needs curly braces").
  Declare it, then assign it.
- No passing structs by value. Prefer `uint8_t`, and avoid large locals.
- Build warnings show up only in the GBDK build, not in gcc. Keep it
  warning-free.
- **ROM is full.**
  - Bank 0 (0x0000-0x3FFF) has ~88 bytes free, and 0x4000-0x7FFF is ~68%
    used.
  - Nothing is banked yet: code and data are linked as one flat 32 KB.
  - Biggest objects: `levels.o` 8.4 KB, `game.o` 7.5 KB, `main.o` 5 KB,
    `world.o` 2 KB, `tiles.o` 1.3 KB.
  - Banking is step 1 of the current plan.
- **WRAM is nearly full** (about 1.4 KB of 8 KB left).
  - The big users are the pathfinding `dist[]`/queue (~4 KB) and undo
    (`UNDO_DEPTH` x `GameState`).
  - Every byte added to `GameState` costs 33+ bytes of RAM.
- Guards' walking distances (`dist[]`) are cached per target tile. Call
  `game_map_changed()` whenever what guards can walk on changes without a
  change of map (e.g. a switch cell flipping).
- Changing tiles while the screen is being drawn shows them half-drawn
  for a frame. Animations must write during vertical blank only, and only
  about 12 tiles fit (see `batch_flush()` in `main.c`).
- VRAM background tiles:
  - The first `BKG_TILE_COUNT` (61) are built in (`src/tiles.c`, generated
    by `tools/gen_tiles.py`).
  - Painted art uses the rest (195 slots per map).
- The font is `0-9 A-Z : ! - . > ?` only (`FONT_CHARS` in `gen_levels.py`).

## Conventions

- Levels must stay solvable, and the solver proves it. Each level teaches
  something; add a `required[]` entry for any new mechanic a level is
  built around.
- Plain-language error messages in the editor's problem list
  (`validate_level()` in `gen_levels.py`).
- The user paints levels by hand in the editor. Don't repaint or
  restructure their levels or art beyond what a task needs, and say so
  when you touch them.
- Commit generated files (`src/levels.c`, `src/tiles.c`) together with
  their sources.
- Keep `README.md` up to date with player-visible and editor-visible
  changes.
