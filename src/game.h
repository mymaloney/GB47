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
} LevelDef;

/* ---- Dynamic state (lives in RAM, snapshotted for undo) ---- */

typedef struct {
    uint8_t x, y;
    uint8_t facing;
    uint8_t next_wp;         /* index of the waypoint it is walking toward */
} Guard;

typedef struct {
    uint8_t  px, py;
    uint8_t  status;
    uint8_t  guard_count;
    uint16_t turn;
    Guard    guards[MAX_GUARDS];
} GameState;

/* Load a level: builds the static map and initialises *s. */
void game_load(const LevelDef *lvl, GameState *s);

/* Apply one player action. Returns 1 if a turn was spent (state changed),
 * 0 if the action was illegal (e.g. walking into a wall) and nothing moved. */
uint8_t game_step(GameState *s, uint8_t action);

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
