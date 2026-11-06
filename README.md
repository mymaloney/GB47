# GB47 — Game Boy stealth puzzle

A top-down, turn-based stealth puzzle game for the original Game Boy.
Enemies move only when you move. The plan is to combine traditional
stealth with social stealth (disguises, dialogue) and combat.

![Starting screens of the three prototype levels](docs/preview.png)

*Rendered from the real tile data and game logic, not an emulator capture.*

## Current state: prototype

- One-screen levels on a 20×16 tile grid, plus a 2-row HUD
- Guards patrol waypoint loops, or stand still as sentries
- Vision cones (4 tiles deep, 3 wide) blocked by walls and drawn on the map
- Getting caught, reaching the exit, 32 turns of undo, restart
- 3 levels, each checked solvable by an automated solver

### Controls

| Button | Action |
|---|---|
| D-pad | Move one tile (spends a turn) |
| A | Wait one turn / next level after escaping |
| B | Undo |
| START | Restart level |

### Turn order

1. Player moves or waits.
2. If standing on the exit, the player escapes.
3. If any guard sees the player, they're caught.
4. Each guard takes one step along its patrol.
5. If a guard walks into the player, or now sees them, they're caught.

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

### Rule tests (no GBDK needed)

```bash
make test
```

This compiles the game rules with `gcc`, runs unit tests, and runs a
breadth-first solver on every level. The solver prints each level's
shortest solution and fails if any level is unsolvable. **Run it after
every level edit.**

## Code layout

| Path | What it is |
|---|---|
| `src/game.c`, `game.h` | All game rules: turns, patrols, vision, undo. No hardware access, so it also compiles on a PC. |
| `src/levels.c` | Level data as ASCII maps plus guard waypoints. |
| `src/main.c` | Game Boy side: input, drawing the map/sprites/HUD. Only reads `GameState`. |
| `tools/gen_tiles.py` | Turns ASCII pixel art into `src/tiles.c`/`tiles.h`. Run `make tiles` after editing the art. |
| `tests/test_game.c` | Unit tests and the level solver. |

Keep rules in `game.c` and presentation in `main.c`. That split is what
makes undo, the solver, and PC testing possible.

## Roadmap

1. ~~Grid movement, patrolling guard, vision, detection, undo~~
2. **Write 5 or more pure-stealth levels and find out what makes them fun**
3. Load levels exported from [Tiled](https://www.mapeditor.org/) instead of ASCII
4. Takedowns: attack a guard from behind; bodies are evidence other guards can discover
5. Disguises: uniforms grant access to zones; some NPCs see through them
6. Dialogue: talking costs a turn and changes a guard's state (sends them away, distracts them)
7. Title screen, save data (battery SRAM), music (hUGETracker)

Introduce each new mechanic with its own teaching level, and let the
solver keep every level honest.
