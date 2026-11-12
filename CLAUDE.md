# GB47: notes for coding agents

A turn-based stealth puzzle game for the original Game Boy (GBDK-2020 4.3.0,
SDCC, MBC5), plus a browser level editor. Read `README.md` for the player
and editor view. `docs/zones-plan.md` (zones, then disguises made in the
editor) and `docs/dialogue-plan.md` are finished plans, kept for their
reasoning.

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
- `src/levels.c`, `levels_maps.c`, `levels_art.c`, `levels.h`:
  **generated** from `levels/levels.json` by `tools/gen_levels.py`, one
  file per ROM bank. Never edit them by hand. The JSON format is
  documented at the top of `gen_levels.py`.
- `src/far.c/h`: the ROM bank layout (read the top of `far.h` first) and
  `far_read()`.
- Characters: guards, dogs and anyone else are all `GuardDef`/`Guard`
  (the code and `levels.json` still say "guards"). Talking (`verb`,
  `script`), `CHAR_NO_TAKEDOWN`, `CHAR_NO_SIGHT` (no cone), the `enforcer` mask
  and the `look` (sprites, presentation only) are per character. Looks
  are `tilesets/characters.png` (6 frames per row, names in
  `characters.json`), generated into `levels.c` as `look_tile_data` and
  loaded into sprite VRAM after the built-in sprites.
- Scripts and switch cells: the rules are in `game.c` (`run_script`,
  `game_talk_target`, `OP_*` in `game.h`), the bytes come from
  `compile_script()` in `gen_levels.py`, and `main.c` only shows the
  message box. A new script step needs: an `OP_*`, a case in `run_script`,
  `compile_script` + `validate_script`, the editor's `STEP_KINDS`, and a
  test.
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
- **ROM banks** (MBC5, 64 KB; details in `src/far.h`):
  - Bank 0 (2.6 KB free): files with no `#pragma bank`: `game.c`,
    `world.c`, `art.c`, `trans.c`, `far.c`, plus GBDK. It can't overflow
    into bank 1 any more, so this is the tight one: **new rules code goes
    here, and there isn't much room.**
  - Bank 1 (6.0 KB free): `main.c`, `tiles.c`, `levels.c` (the level
    table, scripts, text). Always mapped, like bank 0.
  - Banks 2 and 3 (12-14 KB free): map cells and painted art. Only
    `far_read()` and `art_row()` map them in, and only code in bank 0 may
    switch banks.
  - A pointer into bank 2 or 3 (`LevelDef.map`, `MapArt.rle`/`rows`,
    `art_tile_data`) must never be read directly on the Game Boy.
- **Work RAM**: about 5.4 KB of 8 KB free. The current map's cells and the
  pathfinding arrays and the zone of each cell are in cartridge RAM
  (0xA000-0xBFFF, fixed addresses in `game.c`); all 8 KB of it is used.
  - Every byte added to `GameState` still costs 33+ bytes (undo history).
- **Zones** decide who may be where. Each cell holds a zone number
  (0-15): `"zone_map"` per map in the JSON, run-length data in
  `levels_maps.c`, unpacked by `game_use`. Each level has one rule table
  (`"zones"` in the JSON, `LevelDef.zone_rules` pointing at
  `zone_rules<n>` in `levels.c`): two bytes per zone, two bits per
  disguise (`ACCESS_*`), room for `DISGUISE_MAX` 8 outfits. Guards enforce
  the rule of the zone the player stands in (`game_access`: an array
  read, a shift, a mask); `GuardDef.enforcer` is the only per-guard
  exception. Old `"access"` layers are converted by `upgrade_access()`.
  Never assume three outfits: loop over `UNIFORMS` / `A.uniforms`.
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
- VRAM sprite tiles: the built-in ones (`SPRITE_TILE_COUNT`, 4: stripped
  body, cursor, noise), then 6 per look, so 42 looks at most, the
  player's included (`validate_all` checks).
- **Disguises** are data: `"disguises"` in `tilesets/characters.json`
  (name + look; the first is always "suit", your own clothes; at most
  `DISGUISE_MAX` 8). `gen_levels.UNIFORMS` is read from it (and reloaded
  in place by `forget_tilesets()`); `levels.h` gets `DISGUISE_TYPES` and
  `disguise_look[]`. Only `DISGUISE_NONE` (0) is fixed in `game.h`. The
  player is drawn with its disguise's look, facing the way it last moved
  (`player_facing` in `main.c`, `P.facing` in the editor's playtest: only
  looks, so not in `GameState`, and undo leaves it). A character's
  `"uniform"` is a disguise name or `"none"`; old levels called the suit
  `"none"` in zone rules and enforcer tags (`upgrade_suit()`). Characters
  always store their `"look"`. The editor's Disguises panel goes through
  `/api/disguises` (`change_disguises()` in `server.py`).
- The font is `0-9 A-Z : ! - . > ?` only (`FONT_CHARS` in `gen_levels.py`),
  so dialogue is upper-cased. Lower case would cost 26 of the art slots.
- Switch cells are the characters `0`-`7` in the map the rules read
  (`cell_rows()` in `gen_levels.py`); `levels.json` keeps the cell's
  normal character there.

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
