/*
 * solver.h - Breadth-first level solver, shared by the tests and the
 * level editor. Runs on a PC (uses malloc; far too big for the Game Boy).
 */
#ifndef SOLVER_H
#define SOLVER_H

#include "../src/world.h"

/* Mechanics the solver may use. Switch one off to check a level needs it. */
#define SOLVE_USE_TAKEDOWN 1
#define SOLVE_USE_DRAG     2
#define SOLVE_USE_UNIFORM  4
#define SOLVE_USE_ITEMS    8
#define SOLVE_USE_ALL      15

#define SOLVE_THROW 0x80      /* SolveStep.action = SOLVE_THROW | item */

typedef struct {
    uint8_t action;           /* ACT_*, or SOLVE_THROW | item */
    uint8_t tx, ty;           /* throw target */
} SolveStep;

/* Find a shortest solution to the level starting at table[first] (with
 * all its linked maps). Returns its length in turns, 0 if the level can't
 * be beaten, -1 if the search got too big. Up to max_steps steps are
 * written to steps (may be NULL). *states (may be NULL) gets the number
 * of distinct situations explored. */
int solver_run(const LevelDef *table, uint8_t first, int use, SolveStep *steps,
               int max_steps, int *states);

/* Text form: U R D L W moves/wait, u r d l drag/push, TAKEDOWN, UNIFORM,
 * and e.g. S8,6 = stone thrown at (8,6), B = bottle. */
void solver_format(const SolveStep *steps, int len, char *out, int outlen);

#endif
