# Next task: numbered zones with a rule table

Status: **not started**. A handoff spec. Read `CLAUDE.md` first.

## Why

Access is stored today as one byte per cell with two bits per outfit
(`ACCESS_*` in `src/game.h`). That caps the game at 4 outfits, and the
designer paints one layer per outfit. The user wants more outfits and a
model closer to how levels are thought about: a nightclub has a dance
floor, a backstage and a security office, and each has a rule per outfit.

So: paint **zones**, and give each zone a **rule per outfit**.

## What exists now (all to be replaced or adapted)

- `src/game.h`: `ACCESS_LEGAL/TRESPASS/HOSTILE`, `ACCESS_DEFAULT`,
  `LevelDef.access` (run-length `(run, byte)` pairs, 0 = default),
  `GuardDef.enforcer` (bit per disguise), `DISGUISE_TYPES` = 3.
- `src/game.c`: `access[]` (2048 bytes of cartridge RAM at 0xB800 on the
  Game Boy), unpacked in `game_use()` with `far_read_rle()`;
  `game_access()`, `regard_at()`, `game_regard()`; used by
  `game_compute_vision()`, `player_spotted()` and the two "guard walks
  into you" checks.
- `tools/gen_levels.py`: `ACCESS`, `ACCESS_DEFAULT`, `access_rows()`,
  `access_bytes()`, `access_rle()`, `access_is_default()`,
  `validate_access()`, `upgrade_attitude()`; JSON `"access"` per map is
  `{outfit: rows of L/T/H, or one letter}`.
- `tools/editor/game_lib.py`: builds `LevelDef.access`.
- `tools/editor/editor.js`: `accessRows()`, `drawAccess()`,
  `setAccessCells()`, `accessRoom()` (room fill), the `#access-bar`
  toolbar, `accessOutfit`.
- Tests: `test_access` and `dz_level` in `tests/test_game.c`; the
  "access:" block in `tests/test_editor.py`.

## Target design

**Rules (`game.h` / `game.c`)**

- The per-cell byte becomes a **zone number** (0-15 is plenty; allow
  `MAX_ZONES` 16). Storage, `far_read_rle()` and the RAM copy stay as
  they are: only the meaning of the byte changes.
- `LevelDef` gains a rule table:
  `uint8_t zone_rules[MAX_ZONES][ZONE_RULE_BYTES]`, two bits per outfit
  (same `ACCESS_*` values). With `DISGUISE_MAX` 8, that is 2 bytes per
  zone, 32 bytes per map. Keep it in the level table (bank 1, always
  mapped), not in a data bank.
- `game_access(s, x, y)` becomes: zone = `access[cell]`, then the two
  bits for `s->disguise` from `level->zone_rules[zone]`. Nothing else in
  the rules changes: `regard_at()`, vision and detection already go
  through `game_access()`.
- `LevelDef.access == 0` keeps meaning "everything is zone 0". Zone 0's
  default rule is today's default: suit trespassing, uniforms legal.
- `GuardDef.enforcer` is already a bit per disguise: 8 outfits fit.
- Raise `DISGUISE_TYPES` only when a new outfit is actually added (it
  needs a player sprite in `tools/gen_tiles.py` and a name in
  `UNIFORMS`). This task is the storage and the editor, not new outfits,
  unless the user asks for one.

**Data (`levels.json`, `gen_levels.py`)**

```json
"zones": [
  {"name": "dance floor", "rules": {"none": "L", "guard": "L", "officer": "L"}},
  {"name": "backstage",   "rules": {"none": "T"}},
  {"name": "security",    "rules": {"none": "H", "guard": "L", "officer": "L"}}
],
"zone_map": ["0000111122...", ...]
```

- `zones` per map; a missing outfit in `rules` uses `ACCESS_DEFAULT`.
- `zone_map`: one character per cell (hex digit = zone index), rows like
  the map; omitted when every cell is zone 0.
- `upgrade()` must convert today's `"access"` layers: group cells by
  their (suit, guard, officer) triple, make one zone per distinct triple
  (name them "zone 1"...), write `zones` + `zone_map`, drop `access`.
  Level 6 (DRESS CODE) is the only level with a painted layer: it becomes
  two zones (left room, hall). Check it still needs its disguise, 32
  turns.
- Validation, in plain words: at most `MAX_ZONES` zones, unique non-empty
  names, rules are L/T/H for known outfits, `zone_map` is the map's size
  and only uses zones that exist.
- `dumps()`: write `zones`/`zone_map` only when not the default.

**Editor**

- Replace the per-outfit Access toolbar with a **Zones** panel: a list of
  zones (name, colour swatch, and a small L/T/H picker per outfit), plus
  Add / Delete (deleting repaints its cells as zone 0).
- Painting: select a zone, click or drag; keep Shift+click room fill
  (`accessRoom()` works unchanged on zone indices) and Fill map.
- Overlay: tint by **zone colour** with the zone's name drawn once per
  region, or keep the green/amber/red tint "as seen in outfit X" as a
  second view tied to the cone-preview outfit. Do both if cheap; the
  per-outfit view is the one that answers "is this level fair?".
- `resizeMap()` must resize `zone_map`.
- The character panel's "Sees through" list should come from
  `A.uniforms`, which it already does.

## Steps

1. Rules + tests: zone byte and rule table in `game.h`/`game.c`; rewrite
   `test_access` and `dz_level` for zones (keep every assertion's intent).
2. `gen_levels.py`: new JSON, `upgrade()` from `"access"`, validation,
   generated C (`zone_rules` in `levels.c`, zone bytes in
   `levels_maps.c`). Convert `levels/levels.json` by load + save.
3. `game_lib.py` mirror (`LevelDef` layout changes: `editor_info()` will
   refuse to start until it matches) and `tests/test_editor.py`.
4. Editor UI.
5. Docs: `README.md` (Disguises / Access section, editor section),
   `CLAUDE.md` (the Access bullet), the format docstring in
   `gen_levels.py`.

## Constraints to respect

- Bank 0 has about 2.7 KB free; this change should cost well under 100
  bytes of rules code (one extra table lookup).
- Cartridge RAM is full; the design above needs no more.
- `game_access()` is called for every tile of every cone on each redraw:
  keep it to an array read, a shift and a mask. No multiplies.
- `make test`, `make test-editor`, and a warning-free GBDK build before
  each commit. Every level must solve to the same length as now:
  20, 24, 36, 19, 24, 32, 68, 34, 84.
- Don't change the user's levels beyond the format conversion, and say so
  if anything else has to move.

## Open questions for the user

1. Is 16 zones per map enough?
2. Should zones be per map (as above) or shared by all maps of a level?
   Per map is simpler; shared saves retyping rules in multi-map levels.
3. Levels 7 and 9 had guards hostile to the suit before access zones;
   they currently play as trespassing. Restore the big cones there?
4. Which new outfits are wanted first (e.g. techie)? Each needs a player
   sprite and a uniform name.
