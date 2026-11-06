/*
 * game.h - Pure game rules. No Game Boy hardware access in here.
 *
 * Everything in game.c is deterministic and depends only on the level data
 * and the GameState passed in. That gives us:
 *   - cheap undo (a GameState is ~30 bytes; just snapshot it),
 *   - puzzles that always play out the same way,
 *   - the ability to compile and test the rules on a PC (see tests/).
 */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>

/* Play field: one screen, no scrolling. 20x18 tiles total, bottom 2 = HUD. */
#define MAP_W 20
#define MAP_H 16

#define MAX_GUARDS    4
#define MAX_WAYPOINTS 6
#define VISION_RANGE  4   /* how many tiles ahead a guard can see */
#define UNDO_DEPTH    32  /* turns of undo history kept in RAM */

/* Directions. Order matters: sprite tiles are laid out in this order. */
#define DIR_UP    0
#define DIR_RIGHT 1
#define DIR_DOWN  2
#define DIR_LEFT  3

/* Player actions fed into game_step(). */
#define ACT_UP    DIR_UP
#define ACT_RIGHT DIR_RIGHT
#define ACT_DOWN  DIR_DOWN
#define ACT_LEFT  DIR_LEFT
#define ACT_WAIT  4

/* Throwables. Stats live in item_defs[] (game.c), so items can differ
 * later (a louder bottle, a coin that rolls...). */
#define ITEM_TYPES  2
#define ITEM_STONE  0
#define ITEM_BOTTLE 1

typedef struct {
    const char *name;        /* max 8 chars for the inventory window */
    uint8_t     range;       /* max throw distance (grid steps) */
    uint8_t     noise;       /* guards within this many steps hear it */
} ItemDef;

extern const ItemDef item_defs[ITEM_TYPES];

/* What a guard is doing. */
#define GUARD_PATROL      0  /* following its route (or standing post) */
#define GUARD_INVESTIGATE 1  /* walking to a noise */
#define GUARD_RETURN      2  /* walking back to where it left its route */

/* Map cells. */
#define CELL_FLOOR 0
#define CELL_WALL  1
#define CELL_EXIT  2

/* Game status. */
#define STATUS_PLAYING 0
#define STATUS_CAUGHT  1
#define STATUS_WON     2

typedef struct {
    uint8_t x, y;
} Point;

/* ---- Static level data (lives in ROM) ---- */

typedef struct {
    uint8_t facing;          /* initial facing; only matters if wp_count == 1 */
    uint8_t wp_count;        /* 1 = stationary sentry */
    Point   wp[MAX_WAYPOINTS]; /* patrol loop; guard starts on wp[0] */
} GuardDef;

typedef struct {
    const char *name;        /* shown in the HUD, max 13 chars */
    /* '#' wall, '.' floor, 'E' exit, 'P' player start */
    const char *rows[MAP_H];
    uint8_t     guard_count;
    GuardDef    guards[MAX_GUARDS];
    uint8_t     items[ITEM_TYPES]; /* starting inventory, by item type */
} LevelDef;

/* ---- Dynamic state (lives in RAM, snapshotted for undo) ---- */

typedef struct {
    uint8_t x, y;
    uint8_t facing;
    uint8_t next_wp;         /* index of the waypoint it is walking toward */
    uint8_t mode;            /* GUARD_PATROL / _INVESTIGATE / _RETURN */
    uint8_t tx, ty;          /* noise being investigated */
    uint8_t hx, hy;          /* where it left its route ("home") */
} Guard;

typedef struct {
    uint8_t  px, py;
    uint8_t  status;
    uint8_t  guard_count;
    uint16_t turn;
    uint8_t  items[ITEM_TYPES];   /* in the state so undo restores them */
    Guard    guards[MAX_GUARDS];
} GameState;

/* Load a level: builds the static map and initialises *s. */
void game_load(const LevelDef *lvl, GameState *s);

/* Apply one player action. Returns 1 if a turn was spent (state changed),
 * 0 if the action was illegal (e.g. walking into a wall) and nothing moved. */
uint8_t game_step(GameState *s, uint8_t action);

/* Is (x, y) a legal target for throwing this item right now?
 * Needs the item in inventory, floor (not wall) at the target, within the
 * item's range in grid steps, not the player's own tile, and a clear line. */
uint8_t game_can_throw(const GameState *s, uint8_t item, uint8_t x, uint8_t y);

/* Throw an item (spends a turn). Every guard within the item's noise range
 * turns to look this turn; the one with the shortest walk goes to the spot,
 * then returns to its route. Returns 0 (nothing happens) if illegal. */
uint8_t game_throw(GameState *s, uint8_t item, uint8_t x, uint8_t y);

/* Grid-step (Manhattan) distance. */
uint8_t game_distance(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

/* Map queries for the current level. */
uint8_t game_cell(uint8_t x, uint8_t y);

/* Can this guard see tile (x, y)? Uses vision cone + line of sight. */
uint8_t game_guard_sees(const Guard *g, uint8_t x, uint8_t y);

/* Fill vis[y][x] with 1 for every tile any guard can currently see. */
void game_compute_vision(const GameState *s, uint8_t vis[MAP_H][MAP_W]);

/* Undo history. game_step does not push automatically; the caller does,
 * so the solver in tests/ can step without touching the history. */
void    undo_clear(void);
void    undo_push(const GameState *s);
uint8_t undo_pop(GameState *s);     /* returns 0 if history is empty */

#endif
