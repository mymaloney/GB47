/*
 * trans.h - Level transitions: the shapes in which the screen dips to
 * black when a level is finished, and comes back on the next one.
 *
 * Only the pattern lives here (no hardware), so the ROM (main.c) and the
 * editor's preview play exactly the same animation. A transition is played
 * on the TRANS_W x TRANS_H grid of screen tiles (the map view plus the two
 * HUD rows) in n = trans_steps() steps: on the way out a tile turns black
 * at step trans_phase(kind, 0, n, ...), on the way in it appears at step
 * trans_phase(kind, 1, n, ...). Tiles change in step order (top to bottom,
 * left to right within a step), a few per frame: steps set the order, not
 * the speed.
 *
 * To add one: a TRANS_* number, its step count and pattern in trans.c, and
 * its name in TRANSITIONS in tools/gen_levels.py (same order).
 */
#ifndef TRANS_H_INCLUDED
#define TRANS_H_INCLUDED

#include <stdint.h>

#define TRANS_CUT       0   /* no animation */
#define TRANS_FADE      1   /* the palette darkens shade by shade */
#define TRANS_WIPE      2   /* a black edge sweeps left to right */
#define TRANS_CURTAIN   3   /* closes from both sides, opens from the middle */
#define TRANS_IRIS      4   /* a circle closes on the player, opens from them */
#define TRANS_DISSOLVE  5   /* tiles drop out in a dither pattern */
#define TRANS_DIAGONAL  6   /* sweeps from the top-left corner */
#define TRANS_BLINDS    7   /* horizontal slats close downward, line by line */
#define TRANS_INTERLACE 8   /* alternate rows wipe in opposite directions */
#define TRANS_COUNT     9

#define TRANS_W 20
#define TRANS_H 18

/* Steps in the animation; 0 = not tile-based (cut, fade). (cx, cy) is the
 * player's screen tile, the centre of the iris: it must be on the grid. */
uint8_t trans_steps(uint8_t kind, uint8_t cx, uint8_t cy);

/* The step (0 .. n-1) at which screen tile (x, y) changes. in = 0: going
 * to black; in = 1: coming back. n is what trans_steps() returned. */
uint8_t trans_phase(uint8_t kind, uint8_t in, uint8_t n, uint8_t x, uint8_t y,
                    uint8_t cx, uint8_t cy);

#endif
