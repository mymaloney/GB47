# Zones with a rule table, then designer-made disguises

Status: **done**, zones and designer-made disguises both. Kept for the
reasoning; `CLAUDE.md` and `README.md` describe how it works now.

How the disguises part was decided: the list is `"disguises"` in
`tilesets/characters.json` (name + look), the first always "suit" (old
levels' "none" in zone rules and enforcer tags is renamed by
`upgrade_suit()`). The player gets a whole 6-frame look (one code path,
the same painter) and is drawn with its Down frame; the three old player
sprites became the looks `player`, `player_guard`, `player_officer`.
Characters' looks used to default from their uniform's name; every
character now stores its look, so renaming a disguise can't change how
anyone looks. Renames are made in the saved levels too; deleting a
disguise any level uses is refused with the level names.

## Why

Access used to be one byte per cell with two bits per outfit, which
capped the game at 4 outfits and meant painting one layer per outfit.
Levels are thought about as places instead: a nightclub has a dance
floor, a backstage and a security office, each with a rule per outfit.
So a level is painted in **zones**, and each zone has a **rule per
outfit** (legal, trespassing or hostile).

## What was built

- **Rules** (`src/game.h`, `src/game.c`): each cell's byte in cartridge
  RAM is its zone (`MAX_ZONES` 16). `LevelDef.zones` is the run-length
  zone map (bank `BANK_MAPS`, 0 = all zone 0) and `LevelDef.zone_rules`
  the level's table, the same pointer in each of its maps: two bytes per
  zone, two bits per outfit, room for `DISGUISE_MAX` 8 outfits. 0 = one
  zone with today's default (suit trespasses, uniforms legal).
  `game_access()` is an array read, a shift and a mask; it cost 37 bytes
  of bank 0.
- **Data** (`levels.json`, `tools/gen_levels.py`): `"zones"` per level
  (`name`, `rules` per outfit, defaults left out), `"zone_map"` per map
  (a hex digit per cell, left out when all zone 0). `upgrade_access()`
  turns the old `"access"` layers into zones: one per combination of
  rules, the commonest one zone 0. Validation in plain words.
- **Editor**: a Zones panel per level (name, a colour, an L/T/H picker
  per outfit, add, delete: a deleted zone's cells go back to the first
  zone, which can't be deleted); the Zones tool (`8`) paints the picked
  zone, Shift+click fills a room, Fill map does the map; the overlay
  shows zones by colour with names, or the rules for the cone-preview
  outfit. `resizeMap()` keeps the zone map.
- **Levels**: converted by load and save only. Levels 1-5 and 7-9 became
  one zone, "everywhere", hostile to the suit (as the user had painted
  them); level 6 two zones ("zone 1": the hall, hostile to the suit;
  "zone 2": the room on the left). Every level solves in the same number
  of turns as before: 20, 24, 56, 25, 24, 32, 68, 36, 89.

## Disguises made by the level designer (done)

The user's decision: disguises are made in the editor, including how
they look. This is a bigger change than adding one outfit.

- Disguises stop being the fixed `DISGUISE_NONE/GUARD/OFFICER`. They
  become a list in data, like character looks: `tilesets/disguises.json`
  (or a `"disguises"` list beside the looks in
  `tilesets/characters.json`) with a name and a look for each. Disguise 0
  stays "your own suit". Cap at `DISGUISE_MAX` 8 (the enforcer mask and
  two rule bytes per zone).
- Appearance: the player's sprite per disguise is a built-in tile today
  (`SPR_PLAYER + disguise`, `tools/gen_tiles.py`). Move it to the painted
  looks in `tilesets/characters.png`, so the existing "Edit look / New
  look" pixel painter edits it. A disguise needs at least one player
  frame; decide whether the player gets the 6-frame row characters have
  (it has no facing today: one frame is enough, but using a whole look
  keeps one code path).
- A character's `uniform` becomes "which disguise its body gives" (a
  name from the list, or none). Its own look stays separate.
- `game.h`: `DISGUISE_TYPES` becomes a count generated into `levels.h`;
  `gen_levels.py` `UNIFORMS` is read from the data file;
  `GameState.disguise` is unchanged (an index).
- Editor: a Disguises panel (add, rename, delete, edit look); the Zones
  panel's rule pickers, the "Sees through" checkboxes, the uniform
  dropdown and the cone-preview outfit list all read that list. Deleting
  a disguise that levels use must be refused with a plain message naming
  the levels.
- Sprite VRAM: looks are limited to 41 in total; each disguise look
  counts toward that (`validate_all` already checks).
- Zones already loop over the outfit list everywhere; keep it that way.

Constraints: bank 0 has about 2.6 KB free; `make test`, `make
test-editor` and a warning-free GBDK build before each commit; every
level keeps its solution length unless the user changes the level.
