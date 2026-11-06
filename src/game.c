/*
 * game.c - Turn rules for the stealth puzzle prototype.
 *
 * One turn:
 *   1. The player acts (move one tile or wait).
 *   2. Standing on the exit wins immediately.
 *   3. If a guard can see the player now, the player is caught.
 *   4. Every guard takes one patrol step (in guard order).
 *   5. If a guard walks into the player or can see them now, they're caught.
 *
 * Kept free of hardware calls so it compiles with both SDCC (GBDK) and gcc.
 */
#include "game.h"

static uint8_t         map[MAP_H][MAP_W];
static const LevelDef *level;

static const int8_t dir_dx[4] = { 0, 1, 0, -1 };
static const int8_t dir_dy[4] = { -1, 0, 1, 0 };

/* ------------------------------------------------------------------ */
/* Level loading                                                       */
/* ------------------------------------------------------------------ */

void game_load(const LevelDef *lvl, GameState *s)
{
    uint8_t x, y, i;

    level = lvl;
    s->px = 1;
    s->py = 1;

    for (y = 0; y < MAP_H; y++) {
        const char *row = lvl->rows[y];
        for (x = 0; x < MAP_W; x++) {
            char c = row[x];
            if (c == '#') {
                map[y][x] = CELL_WALL;
            } else if (c == 'E') {
                map[y][x] = CELL_EXIT;
            } else {
                map[y][x] = CELL_FLOOR;
                if (c == 'P') {
                    s->px = x;
                    s->py = y;
                }
            }
        }
    }

    s->status      = STATUS_PLAYING;
    s->turn        = 0;
    s->guard_count = lvl->guard_count;
    for (i = 0; i < lvl->guard_count; i++) {
        const GuardDef *d = &lvl->guards[i];
        Guard          *g = &s->guards[i];
        g->x       = d->wp[0].x;
        g->y       = d->wp[0].y;
        g->facing  = d->facing;
        g->next_wp = (d->wp_count > 1) ? 1 : 0;
    }
}

uint8_t game_cell(uint8_t x, uint8_t y)
{
    if (x >= MAP_W || y >= MAP_H) return CELL_WALL;
    return map[y][x];
}

/* ------------------------------------------------------------------ */
/* Vision                                                              */
/* ------------------------------------------------------------------ */

/* Bresenham walk from (x0,y0) to (x1,y1); 1 if no wall in between.
 * The endpoints themselves are not tested. */
static uint8_t line_of_sight(int8_t x0, int8_t y0, int8_t x1, int8_t y1)
{
    int8_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int8_t dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int8_t sx = x0 < x1 ? 1 : -1;
    int8_t sy = y0 < y1 ? 1 : -1;
    int8_t err = dx - dy;
    int8_t e2;

    for (;;) {
        e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
        if (x0 == x1 && y0 == y1) return 1;
        if (map[y0][x0] == CELL_WALL) return 0;
    }
}

/* Cone shape: 1 tile wide right in front of the guard, 3 tiles wide
 * from distance 2 up to VISION_RANGE.
 *
 *          . . .        distance 4
 *          . . .        distance 3
 *          . . .        distance 2
 *            .          distance 1
 *            G  (facing up)
 */
uint8_t game_guard_sees(const Guard *g, uint8_t x, uint8_t y)
{
    int8_t rx = (int8_t)x - (int8_t)g->x;
    int8_t ry = (int8_t)y - (int8_t)g->y;
    int8_t fwd, side;

    switch (g->facing) {
    case DIR_UP:    fwd = -ry; side =  rx; break;
    case DIR_DOWN:  fwd =  ry; side = -rx; break;
    case DIR_LEFT:  fwd = -rx; side = -ry; break;
    default:        fwd =  rx; side =  ry; break; /* DIR_RIGHT */
    }

    if (fwd < 1 || fwd > VISION_RANGE) return 0;
    if (side < 0) side = -side;
    if (side > (fwd >= 2 ? 1 : 0)) return 0;
    if (game_cell(x, y) == CELL_WALL) return 0;

    return line_of_sight((int8_t)g->x, (int8_t)g->y, (int8_t)x, (int8_t)y);
}

void game_compute_vision(const GameState *s, uint8_t vis[MAP_H][MAP_W])
{
    uint8_t i, x, y;
    int8_t  fwd, side, tx, ty;

    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < MAP_W; x++)
            vis[y][x] = 0;

    for (i = 0; i < s->guard_count; i++) {
        const Guard *g  = &s->guards[i];
        int8_t       fx = dir_dx[g->facing];
        int8_t       fy = dir_dy[g->facing];
        /* the "side" axis is the forward axis rotated 90 degrees */
        int8_t       lx = -fy;
        int8_t       ly = fx;

        for (fwd = 1; fwd <= VISION_RANGE; fwd++) {
            for (side = -1; side <= 1; side++) {
                tx = (int8_t)g->x + fx * fwd + lx * side;
                ty = (int8_t)g->y + fy * fwd + ly * side;
                if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) continue;
                if (game_guard_sees(g, (uint8_t)tx, (uint8_t)ty))
                    vis[ty][tx] = 1;
            }
        }
    }
}

static uint8_t player_spotted(const GameState *s)
{
    uint8_t i;
    for (i = 0; i < s->guard_count; i++)
        if (game_guard_sees(&s->guards[i], s->px, s->py)) return 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Guards                                                              */
/* ------------------------------------------------------------------ */

static uint8_t guard_at(const GameState *s, uint8_t x, uint8_t y, uint8_t skip)
{
    uint8_t i;
    for (i = 0; i < s->guard_count; i++) {
        if (i == skip) continue;
        if (s->guards[i].x == x && s->guards[i].y == y) return 1;
    }
    return 0;
}

/* Direction from (x,y) toward p. Horizontal first, then vertical.
 * (Takes a pointer: SDCC is happier without structs passed by value.) */
static uint8_t dir_toward(uint8_t x, uint8_t y, const Point *p)
{
    if (p->x > x) return DIR_RIGHT;
    if (p->x < x) return DIR_LEFT;
    if (p->y > y) return DIR_DOWN;
    return DIR_UP;
}

/* Returns 1 if this guard bumped into the player. */
static uint8_t guard_act(GameState *s, uint8_t idx)
{
    Guard          *g = &s->guards[idx];
    const GuardDef *d = &level->guards[idx];
    uint8_t         dir, nx, ny;
    const Point    *target;

    if (d->wp_count < 2) return 0;  /* stationary sentry */

    target = &d->wp[g->next_wp];
    dir    = dir_toward(g->x, g->y, target);
    nx     = g->x + dir_dx[dir];
    ny     = g->y + dir_dy[dir];
    g->facing = dir;

    if (nx == s->px && ny == s->py) return 1;
    if (game_cell(nx, ny) == CELL_WALL || guard_at(s, nx, ny, idx))
        return 0;   /* blocked: wait this turn */

    g->x = nx;
    g->y = ny;

    /* Reached the waypoint: pick the next one and turn to face it, so the
     * cone always shows where the guard is about to walk. */
    if (g->x == target->x && g->y == target->y) {
        g->next_wp++;
        if (g->next_wp >= d->wp_count) g->next_wp = 0;
        g->facing = dir_toward(g->x, g->y, &d->wp[g->next_wp]);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Turn                                                                */
/* ------------------------------------------------------------------ */

uint8_t game_step(GameState *s, uint8_t action)
{
    uint8_t i;

    if (s->status != STATUS_PLAYING) return 0;

    if (action < 4) {
        uint8_t nx = s->px + dir_dx[action];
        uint8_t ny = s->py + dir_dy[action];
        if (game_cell(nx, ny) == CELL_WALL) return 0;
        if (guard_at(s, nx, ny, 0xFF)) return 0;  /* TODO: takedowns */
        s->px = nx;
        s->py = ny;
    }

    s->turn++;

    if (game_cell(s->px, s->py) == CELL_EXIT) {
        s->status = STATUS_WON;
        return 1;
    }
    if (player_spotted(s)) {
        s->status = STATUS_CAUGHT;
        return 1;
    }

    for (i = 0; i < s->guard_count; i++) {
        if (guard_act(s, i)) {
            s->status = STATUS_CAUGHT;
            return 1;
        }
    }

    if (player_spotted(s)) s->status = STATUS_CAUGHT;
    return 1;
}

/* ------------------------------------------------------------------ */
/* Undo (ring buffer of snapshots)                                     */
/* ------------------------------------------------------------------ */

static GameState history[UNDO_DEPTH];
static uint8_t   hist_top;    /* next free slot */
static uint8_t   hist_count;

void undo_clear(void)
{
    hist_top   = 0;
    hist_count = 0;
}

void undo_push(const GameState *s)
{
    history[hist_top] = *s;
    hist_top = (hist_top + 1) % UNDO_DEPTH;
    if (hist_count < UNDO_DEPTH) hist_count++;
}

uint8_t undo_pop(GameState *s)
{
    if (hist_count == 0) return 0;
    hist_top = (hist_top + UNDO_DEPTH - 1) % UNDO_DEPTH;
    hist_count--;
    *s = history[hist_top];
    return 1;
}
