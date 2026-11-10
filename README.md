# GB47 — Game Boy stealth puzzle

A top-down, turn-based stealth puzzle game for the original Game Boy.
Enemies move only when you move. The plan is to combine traditional
stealth with social stealth (disguises, dialogue) and combat.

![Starting screens of the three prototype levels](docs/preview.png)

*Rendered from the real tile data and game logic, not an emulator capture.*

## Current state: prototype

- Levels of 1-3 screens across and down; the view cuts from screen to screen
- Guards patrol waypoint loops, or stand still as sentries
- Vision cones (4 tiles deep, 3 wide) blocked by walls and drawn on the map
- Getting caught, reaching the exit, 32 turns of undo, restart
- **Distractions:** open the inventory, pick a throwable, and throw it to
  lure guards away
- **Takedowns and bodies:** grab a guard from an adjacent tile, then drag or
  push the body out of sight
- **Disguises:** take a uniform from a body; each guard is hostile,
  an enforcer, or blind to what you're wearing, which sets its vision cone
- **Building blocks:** doors, windows, railings, security doors, curtains,
  mesh screens and vents, each blocking a different mix of movement,
  sight and throws
- **Linked maps:** a level can be up to 4 maps joined at their edges
- **Tileset painting:** paint a level's look from a tileset image
  (`tilesets/`), separately from what each cell does
- 9 levels, each checked solvable by an automated solver
- **Level editor** with in-editor playtesting and a built-in solver

## Level editor

![The level editor](docs/editor.png)

```bash
make editor
```

This needs only `gcc` and Python 3 (no extra packages). It builds the game
rules into `build/libgame.so`, starts a local server, and opens
`http://127.0.0.1:8047` in your browser. Stop it with Ctrl+C.

- **Art:** pick a tile from the **tileset** under the palette and paint
  with it. It also sets the cell's behaviour if the tile has one (shown
  as a letter on the tile); tick **Look only** to change just the look.
  **Erase art** (`0`) returns cells to the built-in look. The **When
  painted, set behaviour to** menu changes a tile's behaviour. **Auto-paint
  map** repaints the whole map from its walls (see above) (saved to the
  tileset settings). **Show behaviour** labels painted cells whose
  behaviour isn't what that tile normally means.
- **Maps:** each level has tabs Map 1, Map 2, …; **+ Map** adds one (up
  to 4). The **Links** panel joins the current map's edges to other
  maps, with an offset; linked edges are marked on the map with where they
  lead. Tools and guards work on the map you're viewing.
- **Size:** pick how many screens across and down (sizes that won't fit
  in RAM are greyed out). Dashed lines are the screen boundaries; **Zoom**
  fits big maps on screen.
- **Paint** any tile from the palette: `1` wall, `2` floor, `3` exit, or a
  building block by its map letter (`D` door, `W` window, `R` railing,
  `S` security door, `C` curtain, `M` mesh, `V` vent). The hint line says
  what the selected tile blocks. Place the player start with `4`.
- **Guards** (`5`): click to add one (max 4), click to select, drag to
  move. **Route** (`6`): click to add waypoints. Corners are inserted for
  you, so legs stay straight. Drag a point to move it; right-click removes
  it. **Close loop** adds the corner needed to walk back to the start.
- Per guard: facing, uniform, and attitude to each disguise.
- Per level: name, design notes, and starting stones/bottles.
- **Cones preview:** shows every guard's starting cone as seen by a player
  in your own clothes, the GUARD uniform, or the OFFICER uniform.
- **Problems** are listed live, in plain language: missing start, a route
  through a wall, a name too long for the HUD, and so on.
- **Solve** finds the shortest solution and reports which mechanics the
  level actually needs (e.g. "Needs a disguise", "Optional: items").
  **Watch solution** replays it.
- **Playtest** (or press `P`) plays the level with the real rules from
  `src/game.c` and the same screen, screen cuts and HUD as the ROM.
  Keyboard: arrows = D-pad, `Z` = A, `X` = B, `Shift` = SELECT (hold +
  arrows to look at other screens), `Enter` = START, `Esc` = back to
  editing.
- **Undo/redo** (Ctrl+Z / Ctrl+Shift+Z) for every edit. Level list: add,
  copy, reorder, delete.
- **Save** (Ctrl+S) writes `levels/levels.json` and regenerates
  `src/levels.c`; it refuses while any level has problems. **Build ROM**
  runs `make` and shows the output.

Levels live in `levels/levels.json` (format documented at the top of
`tools/gen_levels.py`). You can also edit it by hand; `make` regenerates
`src/levels.c` when the JSON changes.

### Controls

| Button | Action |
|---|---|
| D-pad | Move one tile (spends a turn); you can walk over bodies |
| A | Wait one turn / next level after escaping |
| B | Take down an adjacent guard (`B:TAKEDOWN`), or put on the uniform of the body you're standing on (`B:TAKE UNIFORM`) |
| Hold B + D-pad | Drag a body behind you, or push one in front of you |
| SELECT (tap) | Open inventory |
| Hold SELECT + D-pad | Look at the neighbouring screens; let go to return |
| START | Tap: undo. Hold about a second: restart level |

In the inventory, UP/DOWN chooses an item and A starts aiming. While
aiming, dots mark every square you can hit; move the cursor with the
D-pad and press A to throw. B goes back, SELECT cancels.

### Levels made of several maps

A level can be up to **4 maps**, each up to 2×3 screens (the RAM limit),
joined at their edges. Walk off an edge that's linked to another map
and you step into it at the matching spot.

- Only the map you're in is live. Guards in other maps stand still, and
  bodies stay where you left them, until you come back.
- Crossing takes a turn. Guards in the new map get one look at where you
  arrive, but don't move that turn.
- Your items, disguise and turn count come with you. You can't drag a
  body or be mid-takedown across an edge.
- Undo steps back across crossings too.
- Maps are separate: guards can't see, hear or walk from one map into
  another. Put crossings at natural breaks (corridors, doorways).
- The player always starts in the level's first map; exits can be in any.

A link joins one map's edge to another map's opposite edge, with an
**offset**: which row (for left/right) or column (for up/down) of this map
the other map's first row/column lines up with. Openings on both sides
must line up, and the editor's problem list says exactly where they
don't. In `levels.json` a level has `maps` (each with `map` and `guards`)
and `links` (`from`, `edge`, `to`, `offset`). In `src/levels.c` each map
is one `LevelDef`; a level's maps are consecutive, and `part` and `link`
tie them together. `src/world.c` does the crossing and undo.

### Painting with a tileset

Each map has two layers:

- **Behaviour** (the map letters: wall, floor, door…) decides the rules.
  Unpainted walls draw as flat black.
- **Art** decides the look: which tileset tile is drawn on each cell.
  Cells without art use the built-in graphics for their kind, so levels
  don't need painting at all.

The tileset is `tilesets/dungeon.png` (CC0, 8×8 tiles) with settings in
`tilesets/dungeon.json`. Colours are turned into the Game Boy's 4 shades
by brightness; `thresholds` sets the cut-offs. Each tile can have a
**behaviour** (`"36": "D"` makes tile 36 a door): painting with it sets
the cell's behaviour too, so painting a wall tile makes a wall. Tiles
without one only change the look.

On the Game Boy, each map loads the tiles it uses into video memory when
you enter it (the screen blanks for a moment). Cones and the aiming dots
still have to show on painted cells, so the game makes shaded copies of
each tile as it loads: a tile used only on walls needs 1 of the 195 free
tile slots, one on exits or mesh 3, one on floor 5. The editor shows the
count, and the problem list says if a map uses too many.

**Auto-paint** draws rooms as thin outlines on a black void, from the
map's walls alone. Every level is painted this way; use the editor's
**Auto-paint map** button (or `python3 tools/autopaint.py` for every
level) after changing walls, then touch up by hand. For each wall:

1. floor below: a wall face (tiles 66, 1, 1 repeating along the wall);
2. floor to the right / left: a side wall (16 / 19);
3. floor diagonally below-right / below-left: a top corner (0 / 3);
4. floor above: a bottom edge (1);
5. floor diagonally above-right / above-left: a bottom corner (32 / 35);
6. nothing open around it: left unpainted, which draws flat black.

Floor becomes tile 87, exits stairs (52), doors and one-tile gaps in a
horizontal wall a door (36; a gap keeps behaving as floor), vents a grate
(18). The tile numbers are under `autopaint` in the tileset settings.

**Painting tiles:** double-click a tile in the editor's tileset (or use
**Edit pixels**) to paint it pixel by pixel in the 4 Game Boy shades:
pencil and fill, flip, rotate, shift, copy/paste between tiles, undo,
with previews of the tile in game, repeated 3×3, and inside a guard's
view. The lightest shade shows the floor through. **Save tile** writes it
into the tileset PNG (as greys that convert back to exactly those shades;
the tile's original colours are replaced) and regenerates `src/levels.c`.
**+ 16 blank tiles** adds a row to the sheet without renumbering anything.

To use your own tileset, put the PNG in `tilesets/` and point `image` in
`tilesets/dungeon.json` at it. Keep the same tile positions, or repaint,
since levels refer to tiles by number.

### Building blocks

Every tile blocks some combination of four things:

| Tile | Map | Player & bodies | Guards | Sight | Throws | Use it for |
|---|---|---|---|---|---|---|
| Wall | `#` | ✕ | ✕ | ✕ | ✕ | |
| Floor | `.` | | | | | |
| Exit | `E` | | | | | reaching any exit wins |
| Door | `D` | | | ✕ | ✕ | breaking line of sight between rooms |
| Window | `W` | ✕ | ✕ | | ✕ | guards watching between rooms |
| Railing | `R` | ✕ | ✕ | | | balconies, pits: see and throw across |
| Security door | `S` | ✕ | | ✕ | ✕ | guard-only routes |
| Curtain | `C` | | | ✕ | | hiding spots you can throw through |
| Mesh | `M` | | | | ✕ | see-through screens that stop throws |
| Vent | `V` | | ✕ | ✕ | ✕ | player-only routes |

- Anything standing on a tile that blocks sight is **hidden**: you in a
  vent, curtain or doorway, or a body you've pushed there.
- Bodies move wherever the player can.
- A throw needs a clear path and must land on a tile that doesn't block
  throws. A stone on a railing still draws a guard, who walks up to the
  railing, looks, and goes back.
- Hearing still ignores walls of every kind.

The table lives in `cell_blocks[]` in `src/game.c` (and `TILES` in
`tools/gen_levels.py`, which a test keeps in step). Adding a tile means a
new `CELL_*`, a map letter, a row in both tables, and a graphic in
`tools/gen_tiles.py`.

### Multi-screen levels

![Walking from one screen of a 2x2 level to the next: the view cuts](docs/screens.png)

A level is 1 to 3 screens across and 1 to 3 down (each screen is 20×16
tiles), up to 2,048 tiles in total: 1×1, 2×1, 3×1, 1×2, 2×2, 3×2, 1×3 or
2×3 screens. The tile limit is RAM: guard pathfinding needs one byte per
tile.

- The view shows one whole screen and **cuts** to the next when you step
  across an edge. No scrolling motion.
- While aiming, the view shows the screen the cursor is on.
- Hold SELECT and use the D-pad to look at the neighbouring screens; let
  go to return. A quick tap of SELECT still opens the inventory (it opens
  when you let go).
- Guards on other screens keep moving, and their cones reach across screen
  edges. Look before you step across.
- The HUD is drawn in the window layer, so it never moves.

How it works: the Game Boy's background map is 32×32 tiles and wraps, so
`main.c` draws the current screen into it at the level position modulo 32
and sets the scroll registers to match. `game.c` reads the map straight
from ROM.

### Throwing distractions

![Aiming, the inventory, and a guard investigating](docs/throw.png)

- You can throw at a floor square up to **6 steps** away (counted along the
  grid) with a clear straight line from you. Throwing spends your turn.
- Every guard within 6 steps of where it lands spends its next move
  turning to look at it.
- The guard with the **shortest walk** to the spot (ties: the lower-numbered
  guard) walks over to investigate. A marker shows where it's heading. It
  then walks back to where it left its route and carries on.
- Every other guard goes back to its routine the following turn; sentries
  face their post again.
- Hearing ignores walls; walking around them doesn't.
- Items are part of the game state, so undo gives them back.

Range and noise are set per item in `item_defs` in `src/game.c`. The stone
and bottle are identical for now.

### Takedowns and bodies

![Takedown, dragging the body out of sight, walking over it](docs/takedown.png)

- Standing next to a guard (up, down, left or right) shows `B:TAKEDOWN`.
  If the guard could see you, you'd already be caught, so in practice this
  means from behind or from the side.
- A takedown locks you and the guard in place for **3 turns** (the press
  plus 2 more). Those turns play out automatically so you can watch the
  other guards keep moving. During them the guard can't see, hear or move.
- Afterwards the guard is a **body**. If any guard sees a body, or sees a
  takedown in progress, the alarm goes up and the level is lost.
- Hold B and press a direction next to a body:
  - toward the body: **push** it one tile ahead (blocked by walls, guards
    and other bodies);
  - any other direction: **drag** it into the tile you just left.
- Without B held you walk straight over bodies, so they never block a
  corridor. You can't start a drag while standing on a body.
- Undo rewinds a whole takedown in one step.

Takedown length is `TAKEDOWN_TURNS` in `src/game.h`.

### Disguises

![Hostile cones, taking a uniform, walking past a blind sentry](docs/disguise.png)

You start in your own clothes. Stand on a body and press B to put on its
uniform (one turn). Your sprite changes to show what you're wearing. Each
body has one uniform, and there's no changing back.

Every guard has an attitude to each disguise, and that sets its cone:

| Attitude | Cone | Shown as |
|---|---|---|
| **Hostile** | Huge: 7 tiles deep, widening to 7 tiles across | Shaded |
| **Enforcer** | Normal: 4 deep, 3 across | Shaded |
| **Blind** | Ignores you. Still uses the normal cone to spot bodies, takedowns, and you doing something **suspicious** | Dotted |

Suspicious means: a takedown, dragging or pushing a body, throwing, or
changing clothes. A blind guard who walks into you just waits for you to
move.

Uniforms and attitudes are set per guard in `src/levels.c`. Guards in
levels 1-5 have no uniform and are enforcers to everything. Disguise
types are `DISGUISE_*` in `src/game.h` (own clothes, GUARD, OFFICER).
Cone sizes are `VISION_RANGE`, `HOSTILE_RANGE` and `HOSTILE_HALF`.

### Turn order

1. Player moves, waits, throws, drags/pushes a body, or starts a takedown.
2. If standing on the exit, the player escapes.
3. If any guard sees the player, they're caught; if a guard sees a body or
   a takedown in progress, the alarm goes up.
4. If something was thrown, guards in earshot turn to look (instead of acting).
5. Every other standing guard acts: patrols, walks to a noise, or walks back.
6. Step 3's checks again; a guard walking into the player also catches them.
7. A takedown in progress counts down; when it ends, the guard is a body.

"Sees" depends on each guard's attitude to your disguise (see above).

## Building

### Game Boy ROM

1. Install [GBDK-2020](https://github.com/gbdk-2020/gbdk-2020/releases)
   (4.3.0 or newer). Unpack it to `~/gbdk`, or set `GBDK_HOME`.
2. Build:

   ```bash
   make
   ```

3. Open `build/stealth.gb` in an emulator. [Emulicious](https://emulicious.net/)
   is recommended because it has a debugger; use `make DEBUG=1` for source-level
   debugging. Also test on SameBoy or BGB.

Every push also builds the ROM on GitHub Actions. Download it from the
`stealth-rom` artifact on the run.

### ROM space

```bash
make usage
```

prints how full the ROM banks and RAM are (GBDK's `romusage`); CI prints
it on every build. Levels currently live in the first 32 KB of the
cartridge with the code. Several large multi-map levels will outgrow it,
at which point level data needs to move into switchable ROM banks.

### Rule tests (no GBDK needed)

```bash
make test
```

This compiles the game rules with `gcc`, runs unit tests, and runs a
breadth-first solver on every level. The solver tries every action: moves, waits,
throws, takedowns and drags. It prints each level's shortest solution
(`U R D L W` moves/wait, lowercase `u r d l` drag/push that way,
`TAKEDOWN`, `UNIFORM`, and `S7,5` means "stone at (7,5)", `B` is a bottle) and fails
if any level is unsolvable. A table in `tests/test_game.c` lists what each
level teaches (level 4 needs items; level 5 needs a takedown and moving
the body; level 6 needs a disguise; level 7, the 2×2-screen level, needs
the stone; level 8 needs the vent, checked by turning every vent into
wall). Level 9 is two linked maps. The solver re-runs with that mechanic switched off and fails if
the level can still be beaten. **Run it after every level edit.** The solver uses A* search (the
walking distance to the nearest exit, or through a linked edge to one,
guides it), so even big levels solve in seconds while still finding a
shortest solution. Wide-open multi-map levels with every mechanic in
play can still be too big; corridors and rooms keep the search small. Proving a level
*can't* be beaten still means checking every reachable situation; it
gives up after 2 million and reports "too big to tell".

## Code layout

| Path | What it is |
|---|---|
| `src/game.c`, `game.h` | All game rules: turns, patrols, vision, undo. No hardware access, so it also compiles on a PC. |
| `levels/levels.json` | **Level data** (source of truth): maps, guard routes, uniforms and attitudes, starting items. |
| `src/levels.c`, `levels.h` | Generated from the JSON by `tools/gen_levels.py`. Don't edit. |
| `tools/editor/` | Level editor: `server.py` (local server), `game_lib.py` (Python bridge to the C rules), and the page. |
| `tools/solver.c` | A* solver, shared by the tests and the editor. |
| `tilesets/` | Tileset images and their settings (shade thresholds, tile behaviours). |
| `src/art.c`, `tools/tileset.py` | Painted art: map art layers, shaded copies for cones and aiming, and the tileset loader (with a standard-library PNG reader, `tools/png.py`). |
| `src/main.c` | Game Boy side: input, drawing the map/sprites/HUD. Only reads `GameState`. |
| `tools/gen_tiles.py` | Turns ASCII pixel art into `src/tiles.c`/`tiles.h`. Run `make tiles` after editing the art. |
| `tests/test_game.c` | Unit tests, plus solver checks for every level. |
| `tests/test_editor.py` | Checks the editor's bridge and that `src/levels.c` matches the JSON (`make test-editor`). |

Keep rules in `game.c` and presentation in `main.c`. That split is what
makes undo, the solver, and PC testing possible.

## Roadmap

1. ~~Grid movement, patrolling guard, vision, detection, undo~~
2. ~~Distraction throwing~~, ~~takedowns and bodies~~, ~~disguises~~
3. ~~Level editor~~
4. **Write more levels and find out what makes them fun**
5. Dialogue: talking costs a turn and changes a guard's state (sends them away, distracts them)
6. Title screen, save data (battery SRAM), music (hUGETracker)

Introduce each new mechanic with its own teaching level, and let the
solver keep every level honest.
