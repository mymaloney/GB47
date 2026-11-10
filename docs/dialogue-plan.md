# Next task: NPC dialogue and simple scripting

Status: **not started**. This is a handoff spec. Read `CLAUDE.md` first
for the build, the code layout and the constraints.

## What the user asked for

> Notice the dog I added to level 3. That'll be the first test NPC for
> dialogue. I want the player to walk up to the dog, press B, and get a
> dialogue that says "You pet the dog", triggering the stairs to appear.
> This'll need to add some simple scripting in the editor to allow custom
> scripts off of dialogue like this in the future.

So, in priority order:

1. **Level 3 ("THE SENTRY")**:
   - The player walks up to the dog and presses B.
   - A dialogue box shows "You pet the dog".
   - Then the stairs (the exit) appear. Until then there is no exit.
2. **A general, data-driven scripting system** that hangs off NPC
   dialogue. The dog is the first user of it, not a special case.
3. **Editor support**: place NPCs, write their dialogue and scripts,
   playtest them, and see them in the solver.

## Where things stand

- **The dog** is only painted art: tileset tile **21** (`0x15` in the art
  rows) at cell **(11, 5)** of level 3, map 1. That cell's behaviour is
  floor (`.`), so today the player can walk onto the dog. Tile 21 has no
  entry in `tilesets/dungeon.json` `behaviour`.
  - The dog sits in a small room (cells 9-12 x 4-6). Its doorway is at
    **(10, 7)**, painted as a door (art 36) but behaving as floor.
- **The exit** is `E` at **(16, 13)**, with stairs art 52 (`0x34`).
  - Level 3 currently solves in 27 turns without going near the dog. Its
    patrol guard walks the loop (6,1) → (6,9) → (18,9) → (18,1), and a
    sentry stands at (14,10) facing left.
- **B already does things** in `input_play()` (`src/main.c`): takedown
  if `game_takedown_target()` finds an adjacent guard, else take uniform.
  Hold B + D-pad drags. The HUD's second row shows the prompt
  (`B:TAKEDOWN`, ...).
- **The player has no facing**: "next to" means Manhattan distance 1
  (see `game_takedown_target()`). Use the same rule for talking.
- **The window layer** already does a pop-up: the inventory
  (`open_inventory()` / `draw_inventory()` / `close_inventory()` in
  `main.c`) grows the window up over the map and hides sprites. A
  dialogue box can reuse that pattern.
- **Map cells are read-only ROM strings.** `game_cell()` switches on
  `level->map[...]`. There is no per-game cell state yet.
- **Roadmap** (`README.md`): "Dialogue: talking costs a turn and changes
  a guard's state". This dog task is the first step; talking to guards
  can come later on the same system.

## Step 1 (blocker): make room in ROM

Bank 0 has **88 bytes free**, and the flat 32 KB is ~80% used. The
dialogue code, the script interpreter, the text and the font won't fit
until something moves into switchable banks. Do this first, as its own
commit, and keep every test and level behaving the same.

Things to know:

- The rules read the current map's chars constantly: `game_cell()` from
  the BFS, vision and line of sight. Art RLE and `art_tile_data` are read
  when drawing. Whatever ROM bank holds them must be mapped in while
  that code runs, and code in 0x4000-0x7FFF is swapped out by any bank
  switch.
- WRAM has no room to copy a 2048-cell map into.

Recommended approach (but measure, and pick what works):

- **A. Cartridge RAM as a working copy.**
  1. Switch the cartridge to MBC5+RAM+BATTERY (`-Wm-yt0x1B -Wm-ya1`; the
     roadmap wants save data in SRAM anyway).
  2. Move all level data (maps, art RLE, tile data, scripts, text) into
     switchable ROM banks: `#pragma bank 255` in the generated
     `levels.c`, `BANKREF`/`BANK()`, and `-autobank` / bankpack.
  3. On `game_use()` / map load, copy the current map's chars (and
     anything else hot) into SRAM at 0xA000. The rules read only from
     there.
  4. Optionally move `dist[]` and the BFS queue into SRAM too, which frees
     ~4 KB of WRAM.

  Only the loader touches banks.
- **B. Bank-switching accessors.** Keep the rules in bank 0, put level
  data in a bank, and have bank-0 functions save, `SWITCH_ROM`, read and
  restore. Loop-heavy functions (BFS, vision) switch once around the
  loop. This is more fiddly, because any banked caller of a bank-0 helper
  that switches must get its bank restored.

The PC builds (tests, `libgame.so`) must keep working. For example, wrap
the GB-only parts in `#ifdef __SDCC` or `#ifdef __PORTSM83__`, and on PC
just point at the data. Check `make usage` before and after, and put the
new numbers in `README.md` ("ROM space") and `CLAUDE.md`.

## Step 2: NPCs, talking, and scripts in the rules (`src/game.c`)

Suggested design. Adjust if you find something cleaner, but keep the
rules in `game.c` so undo, the solver and the editor get it for free.

**NPCs** are static per-map objects in `LevelDef`, up to e.g. 4 per map:

- `x, y`;
- what blocks (a dog should block walking for the player and guards, but
  not sight or throws);
- a HUD verb (`PET`, `TALK`, ... shown as `B:PET`);
- a script index.

The dog's look stays as painted art (tile 21), so no sprite is needed.
Later NPCs that move would need sprites; don't build that now.

**Talking** is a new action, e.g. `ACT_TALK` (increase `ACT_COUNT`).
- It's legal when an NPC is next to the player.
- It runs the NPC's script, then `finish_turn()`, so it costs a turn and
  guards move, as the roadmap says.
- Decide whether talking is suspicious. Probably not: petting a dog isn't
  a crime.
- B priority in `main.c`: takedown, then talk, then take uniform. Or show
  whichever applies. They rarely overlap.

**Script state** goes in `GameState`, so undo restores it:
- e.g. `uint8_t flags` (8 per level);
- plus `uint8_t say` (the message to show after this turn, 0 = none,
  cleared at the start of each step).
- Carry `flags` across map crossings exactly like `items`/`disguise` in
  `cross()` (`src/world.c`), so a dog in one map can open a door in
  another.
- Every byte here costs 32+ bytes of WRAM (undo), so keep it small.

**Switch cells** make the stairs appear: a cell that behaves as one thing
while a flag is clear and another once it's set.
- Cheap version: map chars `0`-`7` mean "switch slot k". The `LevelDef`
  gets a small table per map: `{flag, off cell, on cell, off art, on art}`.
- `game_cell()` resolves those chars against `s->flags`. It currently has
  no `GameState`; keep a pointer to the active one, like the existing
  `level` pointer, set in `game_load()`/`game_use()`/`world_activate()`.
- Keep `game_cell()` fast. It's the hottest function in the game.
- For level 3, the exit cell becomes a switch: floor (no art, or floor
  art 87) until the "dog petted" flag is set, then `E` with stairs art
  52.

**Scripts** are small bytecode in ROM, generated by `gen_levels.py` from
JSON. Start with only what the dog needs, but shape it for more:

| Op | Meaning |
|---|---|
| `SAY "text"` | show a message box (text pre-wrapped by `gen_levels.py` to the box width) |
| `SET flag` / `CLEAR flag` | flip a flag (switch cells react) |
| `IF flag` … `ELSE` … `END` | e.g. a different line the second time ("The dog wags its tail") |
| `END` | stop |

Later candidates, listed only so the format leaves room for them:

- `GIVE item n`
- `IF item` / `IF disguise`
- `WIN`
- `ALARM`
- distracting a guard: `SEND guard x,y`, which reuses
  `GUARD_INVESTIGATE`.

For several `SAY`s in a row, record the script position in state, or
queue the message ids. Keep it simple: up to 4 message ids per turn is
plenty.

## Step 3: Game Boy side (`src/main.c`)

- **HUD prompt**: `B:PET` (the NPC's verb) when next to an NPC.
- **Dialogue box**:
  1. After a talk turn, if `state.say` is set, open a window box (like
     the inventory) showing the text.
  2. A (or B) closes it, or shows the next message.
  3. Input is otherwise blocked while it's open: `UI_DIALOGUE` mode.
- **Font**: only `0-9 A-Z : ! - . > ?` exist, so the text shows as
  `YOU PET THE DOG.`. Lowercase (+26 tiles) and `, '` would cut the 195
  art slots per map. **Ask the user** before adding lowercase. Until
  then, upper-case the text when generating and validate characters in
  `gen_levels.py`.
- **Redraw**: after the turn, so the stairs appear (art + cell). A full
  `redraw()` already runs after each action. `load_art()` must have
  loaded the *on* art tile too: `gen_levels.py` lists switch art in the
  map's local tiles, and `slots_for()` should use the cell chars of both
  states (an exit needs 3 slots).

## Step 4: data, generator, validation (`tools/gen_levels.py`)

Extend `levels.json`. Document it in the docstring at the top of
`gen_levels.py`. For example:

```json
"maps": [{
  "map": [...], "art": [...], "guards": [...],
  "npcs": [{"x": 11, "y": 5, "verb": "PET", "blocks": "move",
            "script": [
              {"if": "dog", "then": [{"say": "The dog wags its tail."}],
               "else": [{"say": "You pet the dog."}, {"set": "dog"}]}
            ]}],
  "switches": [{"x": 16, "y": 13, "flag": "dog",
                "off": ".", "on": "E", "off_art": 87, "on_art": 52}]
}]
```

Names for flags (`"dog"`) are friendlier than numbers. Map them to bits
per level and validate:

- at most 8 flags;
- unknown flags;
- text that doesn't fit the box or uses missing font characters;
- an NPC on a wall or a guard route;
- a switch whose `off`/`on` aren't cell chars.

Keep messages plain and in the editor's problem list. A level must still
have at least one exit, but it may be a switch's `on` cell.

## Step 5: solver and tests

- `tools/solver.c` must try `ACT_TALK` (add `SOLVE_USE_TALK` to
  `solver.h`), and must hash the new state bytes. It already hashes the
  whole `World`, so new `GameState` fields are covered. Check
  `solver_format()` prints talks, e.g. `T`.
- `tests/test_game.c`:
  - Unit tests: talking is legal only next to an NPC; it costs a turn;
    `SAY` sets `say`; `SET` flips the flag, and the switch cell becomes
    an exit; undo restores it; flags cross maps; NPCs block movement.
  - Add `{ 3, SOLVE_USE_TALK, "petting the dog" }` to `required[]`, so
    level 3 can't be beaten without the dog.
- Update the ctypes mirror in `tools/editor/game_lib.py` (`GameState`,
  `LevelDef`, new structs) and `editor_info()` in `tools/editor_lib.c`.

## Step 6: editor (`tools/editor/`)

- **NPC tool**:
  - Place it (number key `7` is free) and select it.
  - Properties panel: verb, blocking, and the script.
  - Draw NPCs on the map like guards, with an outline over the painted
    art.
- **Script editor**:
  - A small structured list rather than a text language.
  - Each line is a dropdown (Say / Set flag / Clear flag / If flag …
    else … end) plus its argument.
  - A live preview of how the text wraps in the Game Boy box.
  - Keep it generic, so future NPCs and ops slot in.
- **Switch cells**:
  - A paint tool: "appears when flag … is set".
  - Edit view: a dashed outline plus the on-tile, so hidden stairs are
    visible to the designer.
  - Cones preview / Show behaviour should keep making sense.
- **Playtest**:
  - B talks.
  - The dialogue box draws on the canvas like the ROM's.
  - The play status shows flags.
  - The keys table in `index.html` mentions talking.
- **Solve** shows talks in the solution and offers "needs talking" in
  its mechanics list.
- **Auto-tile/auto-paint** (`tools/autopaint.py`): NPC cells and switch
  cells must not get repainted as plain floor/exit. Leave their art
  alone.

## Step 7: level 3 and docs

- Level 3:
  - Make the dog an NPC with the script above.
  - Turn the exit at (16,13) into a switch on the "dog" flag.
  - Check with the solver that the level is still solvable, and that it
    *needs* the dog. Report the new solution length.
  - Don't otherwise redesign the level. If it becomes unsolvable, or
    trivial, tell the user and propose the smallest change.
- Docs:
  - `README.md`: feature list, controls (B to talk/pet), an editor
    section for NPCs/scripts/switches, the `levels.json` format pointer,
    and the roadmap.
  - `CLAUDE.md`: new constraints and numbers.

## Done when

- [ ] `make test`, `make test-editor` and the ROM build pass in CI, with
      no new SDCC warnings.
- [ ] On the ROM:
  - Walking next to the dog shows `B:PET`.
  - B shows "YOU PET THE DOG." (or mixed case, if the user chose
    lowercase).
  - Closing it leaves stairs at (16,13), and walking onto them wins.
  - Undo hides them again.
- [ ] The same works in the editor's Playtest, and Solve's solution pets
      the dog.
- [ ] A second NPC with a different script can be made entirely in the
      editor, without code changes.
- [ ] `make usage` numbers are recorded, with room left to grow.

## Open questions for the user

1. Lowercase font (costs VRAM art slots) or upper-case text for now?
2. Should talking cost a turn (guards move)? The roadmap says yes.
3. Can guards see the player petting the dog (is talking suspicious)?
   Suggested: no.
4. Should the dog block the player, or be walk-over like bodies?
   Suggested: block.
