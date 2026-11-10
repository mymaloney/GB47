/*
 * world.h - A level made of up to MAX_PARTS maps joined at their edges.
 *
 * Each map (a LevelDef, up to 2x3 screens) keeps its own GameState. Only
 * the map the player is in is "live": guards in the others stand still
 * until the player comes back. Walking off an edge that links to another
 * map moves the player there (one turn); guards in the new map get one
 * look at the arrival spot but don't move that turn. Items, disguise,
 * script flags and the turn counter carry over. A takedown in progress or a body being
 * dragged can't cross.
 *
 * Maps are kept separate because of RAM: guards can't see, hear or walk
 * from one map into another.
 */
#ifndef WORLD_H
#define WORLD_H

#include "game.h"

#define MAX_PARTS 4   /* maps per level */

typedef struct {
    uint8_t   cur;                   /* map the player is in */
    uint8_t   part_count;
    uint8_t   part_def[MAX_PARTS];   /* index of each map in the level table */
    GameState part[MAX_PARTS];       /* only part_count are used */
} World;

/* Bytes of a World that matter (unused parts left out), for comparing and
 * hashing worlds. */
#define WORLD_BYTES(w) ((uint16_t)(sizeof(World) - (MAX_PARTS - (w)->part_count) * sizeof(GameState)))

/* Use this level table from now on, without loading anything: for a World
 * that was made from it earlier (the editor keeps Worlds between requests).
 * world_load does this itself. */
void world_use(const LevelDef *table);

/* Start the level whose first map is table[first]; its other maps are
 * found by following links. */
void world_load(const LevelDef *table, uint8_t first, World *w);

/* Point the rules at the current map again, e.g. after restoring a World
 * from undo. Every world_* call does this itself. */
void world_activate(const World *w);

GameState      *world_state(World *w);          /* the current map's state */
const LevelDef *world_def(const World *w);      /* the current map */

/* Like game_step/game_throw on the current map, but a move off a linked
 * edge crosses into the neighbouring map. */
uint8_t world_step(World *w, uint8_t action);
uint8_t world_throw(World *w, uint8_t item, uint8_t x, uint8_t y);

/* Undo history: one entry per turn = the map that was current plus its
 * state before the turn. (A crossing changes nothing in the map entered
 * that undo needs to put back; see world.c.)
 *
 * Taking a turn is two steps, so a move that turns out to be illegal costs
 * nothing: undo_mark before trying it, undo_commit if it happened. */
void    undo_clear(void);
void    undo_mark(const World *w);     /* remember the situation */
void    undo_commit(void);             /* ...and keep it as an undo step */
#define undo_push(w) (undo_mark(w), undo_commit())
uint8_t undo_pop(World *w);     /* returns 0 if history is empty */

#endif
