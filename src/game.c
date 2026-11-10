/*
 * game.c - Turn rules for the stealth puzzle prototype.
 *
 * One turn:
 *   1. The player acts: move, wait, throw, drag/push a body, talk to an
 *      NPC (which runs its script), or take down an adjacent guard (which
 *      locks the player for TAKEDOWN_TURNS).
 *   2. Standing on the exit wins immediately.
 *   3. If a guard can see the player now, the player is caught; if it can
 *      see a body or a takedown in progress, the alarm goes up.
 *   4. If something was thrown, every guard within earshot spends this turn
 *      turning to look at it. The one with the shortest walk to it will go
 *      and investigate on the following turns.
 *   5. Every other guard acts (in guard order): patrol, investigate a noise,
 *      or walk back to where it left its route.
 *   6. Step 3's checks again.
 *   7. A takedown in progress counts down; at zero the guard is a body.
 *
 * Kept free of hardware calls so it compiles with both SDCC (GBDK) and gcc.
 */
#include <string.h>
#include "game.h"
#include "far.h"

const ItemDef item_defs[ITEM_TYPES] = {
    /* name      range noise */
    { "STONE",   6,    6 },
    { "BOTTLE",  6,    6 },
};

/* On the Game Boy the map's cells are in a ROM bank that isn't normally
 * mapped (far.h), so game_use copies the current map into cartridge RAM
 * and the rules read that. The pathfinding arrays live there too: work
 * RAM has no room for any of them. On a PC, cells points straight at the
 * level data. row_start[y] is y * width, so a tile lookup needs no
 * multiply (slow on the Game Boy). */
#define NO_PATH    0xFF
#define QUEUE_SIZE 1024   /* ring buffer; holds the BFS frontier only */

#ifdef __SDCC
static char    __at(0xA000) map_copy[MAP_MAX_CELLS];
static uint8_t __at(0xA800) dist[MAP_MAX_CELLS];      /* dist[row_start[y] + x] */
static uint8_t __at(0xB000) queue_x[QUEUE_SIZE];
static uint8_t __at(0xB400) queue_y[QUEUE_SIZE];
#define cells map_copy
#else
static const char *cells;
static uint8_t dist[MAP_MAX_CELLS];
static uint8_t queue_x[QUEUE_SIZE];
static uint8_t queue_y[QUEUE_SIZE];
#endif

static const LevelDef *level;
static uint8_t         map_w, map_h;
static uint16_t        row_start[MAP_MAX_H];

static uint8_t cur_flags;               /* what switch cells are read with */
static uint8_t said[MAX_SAID], said_n;  /* messages from the last action */

static const int8_t dir_dx[4] = { 0, 1, 0, -1 };
static const int8_t dir_dy[4] = { -1, 0, 1, 0 };

/* ------------------------------------------------------------------ */
/* Level loading                                                       */
/* ------------------------------------------------------------------ */

void game_use(const LevelDef *lvl)
{
    uint8_t  y;
    uint16_t at = 0;

    map_w = lvl->width;
    map_h = lvl->height;
    for (y = 0; y < map_h; y++, at += map_w) row_start[y] = at;
#ifdef __SDCC
    /* ROM never changes, so the copy is only needed for a different map */
    if (lvl != level) far_read(BANK_MAPS, map_copy, lvl->map, at);
#else
    cells = lvl->map;
#endif
    if (lvl != level) game_map_changed();
    level = lvl;
}

void game_load(const LevelDef *lvl, GameState *s)
{
    uint8_t  x, y, i;
    uint16_t at = 0;

    /* zero everything (incl. padding) so states compare with memcmp */
    memset(s, 0, sizeof(GameState));
    game_use(lvl);
    game_map_changed();     /* a new game: assume nothing about the map */
    cur_flags = 0;
    said_n    = 0;
    s->px = 1;
    s->py = 1;

    for (y = 0; y < map_h; y++)
        for (x = 0; x < map_w; x++, at++)
            if (cells[at] == 'P') {
                s->px = x;
                s->py = y;
            }

    s->status      = STATUS_PLAYING;
    s->guard_count = lvl->guard_count;
    for (i = 0; i < ITEM_TYPES; i++) s->items[i] = lvl->items[i];
    for (i = 0; i < lvl->guard_count; i++) {
        const GuardDef *d = &lvl->guards[i];
        Guard          *g = &s->guards[i];
        g->x       = d->wp[0].x;
        g->y       = d->wp[0].y;
        g->facing  = d->facing;
        g->next_wp = (d->wp_count > 1) ? 1 : 0;
        g->mode    = GUARD_PATROL;
    }
}

/* What each kind of cell blocks; see the table in game.h. */
const uint8_t cell_blocks[CELL_TYPES] = {
    0,                                          /* floor    */
    BLOCK_ALL,                                  /* wall     */
    0,                                          /* exit     */
    BLOCK_SIGHT | BLOCK_THROW,                  /* door     */
    BLOCK_PLAYER | BLOCK_GUARD | BLOCK_THROW,   /* window   */
    BLOCK_PLAYER | BLOCK_GUARD,                 /* railing  */
    BLOCK_PLAYER | BLOCK_SIGHT | BLOCK_THROW,   /* security */
    BLOCK_SIGHT,                                /* curtain  */
    BLOCK_THROW,                                /* mesh     */
    BLOCK_GUARD | BLOCK_SIGHT | BLOCK_THROW,    /* vent     */
};

void game_set_flags(uint8_t flags)
{
    if (flags == cur_flags) return;
    cur_flags = flags;
    game_map_changed();     /* a switch cell may have opened or closed */
}

uint8_t game_cell(uint8_t x, uint8_t y)
{
    char c;
    if (x >= map_w || y >= map_h) return CELL_WALL;
    c = cells[row_start[y] + x];
    switch (c) {
    case '.': return CELL_FLOOR;
    case '#': return CELL_WALL;
    case 'E': return CELL_EXIT;
    case 'D': return CELL_DOOR;
    case 'W': return CELL_WINDOW;
    case 'R': return CELL_RAILING;
    case 'S': return CELL_SECURITY;
    case 'C': return CELL_CURTAIN;
    case 'M': return CELL_MESH;
    case 'V': return CELL_VENT;
    default:
        if (c >= '0' && c < '0' + MAX_SWITCHES) {       /* a switch cell */
            const SwitchDef *sw = &level->switches[c - '0'];
            return (cur_flags & sw->flag) ? sw->on : sw->off;
        }
        return CELL_FLOOR;             /* 'P' */
    }
}

uint8_t game_blocks(uint8_t x, uint8_t y, uint8_t what)
{
    uint8_t b = cell_blocks[game_cell(x, y)], i;

    for (i = 0; i < level->npc_count; i++)      /* an NPC stands in the way */
        if (level->npcs[i].x == x && level->npcs[i].y == y) b |= level->npcs[i].blocks;
    return b & what;
}

uint8_t game_width(void)  { return map_w; }
uint8_t game_height(void) { return map_h; }

uint8_t game_distance(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    return (uint8_t)((x0 > x1 ? x0 - x1 : x1 - x0) +
                     (y0 > y1 ? y0 - y1 : y1 - y0));
}

/* ------------------------------------------------------------------ */
/* Line of sight / vision                                              */
/* ------------------------------------------------------------------ */

/* Bresenham walk from (x0,y0) to (x1,y1); 1 if no tile in between blocks
 * `what` (BLOCK_SIGHT or BLOCK_THROW). The endpoints are not tested. */
static uint8_t line_of_sight(int8_t x0, int8_t y0, int8_t x1, int8_t y1, uint8_t what)
{
    int8_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int8_t dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int8_t sx = x0 < x1 ? 1 : -1;
    int8_t sy = y0 < y1 ? 1 : -1;
    int8_t err = dx - dy;
    int8_t e2;

    if (x0 == x1 && y0 == y1) return 1;
    for (;;) {
        e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
        if (x0 == x1 && y0 == y1) return 1;
        if (game_blocks((uint8_t)x0, (uint8_t)y0, what)) return 0;
    }
}

/* Attitude toward the player's disguise, from the level's attitude string. */
uint8_t game_attitude(const GameState *s, uint8_t idx)
{
    const char *a = level->guards[idx].attitude;
    if (!a) return ATT_ENFORCER;
    switch (a[s->disguise]) {
    case 'H': return ATT_HOSTILE;
    case 'B': return ATT_BLIND;
    default:  return ATT_ENFORCER;
    }
}

/* Cone shapes (guard G facing up). Normal (enforcer) cone:
 *
 *          . . .        distance 4
 *          . . .        distance 3
 *          . . .        distance 2
 *            .          distance 1
 *            G
 *
 * Hostile cone: widens by one tile per side each step (up to HOSTILE_HALF)
 * and reaches HOSTILE_RANGE: widths 1, 3, 5, 7, 7, 7, 7.
 * cone is ATT_HOSTILE or ATT_ENFORCER. */
static uint8_t cone_range(uint8_t cone)
{
    return cone == ATT_HOSTILE ? HOSTILE_RANGE : VISION_RANGE;
}

static int8_t cone_half(uint8_t cone, int8_t fwd)
{
    if (cone == ATT_HOSTILE) return fwd - 1 > HOSTILE_HALF ? HOSTILE_HALF : fwd - 1;
    return fwd >= 2 ? 1 : 0;
}

static uint8_t cone_sees(const Guard *g, uint8_t x, uint8_t y, uint8_t cone)
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

    if (fwd < 1 || fwd > (int8_t)cone_range(cone)) return 0;
    if (side < 0) side = -side;
    if (side > cone_half(cone, fwd)) return 0;
    /* a sight-blocking tile hides whatever stands on it */
    if (game_blocks(x, y, BLOCK_SIGHT)) return 0;

    return line_of_sight((int8_t)g->x, (int8_t)g->y, (int8_t)x, (int8_t)y, BLOCK_SIGHT);
}

uint8_t game_guard_sees(const Guard *g, uint8_t x, uint8_t y)
{
    return cone_sees(g, x, y, ATT_ENFORCER);
}

/* The cone a guard uses for bodies and suspicious acts: its own, except
 * blind guards still watch with the normal one. */
static uint8_t watch_cone(uint8_t att)
{
    return att == ATT_HOSTILE ? ATT_HOSTILE : ATT_ENFORCER;
}

void game_compute_vision(const GameState *s, uint8_t x0, uint8_t y0,
                         uint8_t w, uint8_t h, uint8_t *vis)
{
    uint8_t  i, att, cone, mark, range;
    int8_t   fwd, side, half;
    int16_t  tx, ty;
    uint8_t *v;

    memset(vis, VIS_NONE, (uint16_t)w * h);

    for (i = 0; i < s->guard_count; i++) {
        const Guard *g  = &s->guards[i];
        int8_t       fx, fy, lx, ly;
        if (!GUARD_ACTIVE(g)) continue;
        att   = game_attitude(s, i);
        cone  = watch_cone(att);
        mark  = att == ATT_BLIND ? VIS_WATCHED : VIS_SEEN;
        range = cone_range(cone);
        fx = dir_dx[g->facing];
        fy = dir_dy[g->facing];
        /* the "side" axis is the forward axis rotated 90 degrees */
        lx = -fy;
        ly = fx;

        for (fwd = 1; fwd <= (int8_t)range; fwd++) {
            half = cone_half(cone, fwd);
            for (side = -half; side <= half; side++) {
                tx = (int16_t)g->x + fx * fwd + lx * side;
                ty = (int16_t)g->y + fy * fwd + ly * side;
                /* only tiles inside the requested block */
                if (tx < x0 || ty < y0 || tx >= x0 + w || ty >= y0 + h) continue;
                v = &vis[(uint16_t)(ty - y0) * w + (tx - x0)];
                if (*v >= mark) continue;
                if (cone_sees(g, (uint8_t)tx, (uint8_t)ty, cone)) *v = mark;
            }
        }
    }
}

/* Is the player spotted? suspicious = the player is doing something
 * (takedown, dragging, throwing, changing) that even blind guards notice. */
static uint8_t player_spotted(const GameState *s, uint8_t suspicious)
{
    uint8_t i, att;
    for (i = 0; i < s->guard_count; i++) {
        const Guard *g = &s->guards[i];
        if (!GUARD_ACTIVE(g)) continue;
        att = game_attitude(s, i);
        if (att == ATT_BLIND && !suspicious) continue;
        if (cone_sees(g, s->px, s->py, watch_cone(att))) return 1;
    }
    return 0;
}

/* Does any active guard see a body or a guard being taken down? */
static uint8_t body_spotted(const GameState *s)
{
    uint8_t i, j;
    for (j = 0; j < s->guard_count; j++) {
        const Guard *b = &s->guards[j];
        if (GUARD_ACTIVE(b)) continue;
        for (i = 0; i < s->guard_count; i++)
            if (GUARD_ACTIVE(&s->guards[i]) &&
                cone_sees(&s->guards[i], b->x, b->y,
                          watch_cone(game_attitude(s, i)))) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Pathfinding: breadth-first distances from one tile                  */
/* ------------------------------------------------------------------ */

#define NO_TARGET 0xFF    /* dist_x when dist[] holds nothing usable */

static uint8_t dist_x = NO_TARGET, dist_y;   /* the tile dist[] was filled for */

#define DIST(x, y) dist[row_start[y] + (x)]

void game_map_changed(void)
{
    dist_x = NO_TARGET;
}

/* Fill dist[] with walking distance to (x, y), ignoring guards and the
 * player. Unreachable tiles get NO_PATH. The distances depend only on the
 * map, so they are kept until another tile is asked for: a guard walking
 * to a noise (or back) over several turns searches the map once. */
static void distances_to(uint8_t x, uint8_t y)
{
    uint16_t head = 0, tail = 0;
    uint8_t  cx, cy, nx, ny, nd, d;

    if (x == dist_x && y == dist_y) return;
    dist_x = x;
    dist_y = y;
    memset(dist, NO_PATH, (uint16_t)map_w * map_h);
    DIST(x, y) = 0;
    queue_x[0] = x;
    queue_y[0] = y;
    tail = 1;

    while (head != tail) {
        cx = queue_x[head];
        cy = queue_y[head];
        head = (head + 1) & (QUEUE_SIZE - 1);
        nd = DIST(cx, cy);
        if (nd < NO_PATH - 1) nd++;            /* very long paths saturate */
        for (d = 0; d < 4; d++) {
            nx = cx + dir_dx[d];
            ny = cy + dir_dy[d];
            if (game_blocks(nx, ny, BLOCK_GUARD)) continue;
            if (DIST(nx, ny) != NO_PATH) continue;
            DIST(nx, ny) = nd;
            queue_x[tail] = nx;
            queue_y[tail] = ny;
            tail = (tail + 1) & (QUEUE_SIZE - 1);
            if (tail == head) return;          /* frontier overflow: give up */
        }
    }
}

/* ------------------------------------------------------------------ */
/* Guards                                                              */
/* ------------------------------------------------------------------ */

/* A standing guard (anything but a body) on this tile? */
static uint8_t guard_at(const GameState *s, uint8_t x, uint8_t y, uint8_t skip)
{
    uint8_t i;
    for (i = 0; i < s->guard_count; i++) {
        if (i == skip || s->guards[i].mode == GUARD_BODY) continue;
        if (s->guards[i].x == x && s->guards[i].y == y) return 1;
    }
    return 0;
}

/* Index of a body on this tile, or 0xFF. Bodies never block walking. */
static uint8_t body_at(const GameState *s, uint8_t x, uint8_t y)
{
    uint8_t i;
    for (i = 0; i < s->guard_count; i++) {
        if (s->guards[i].mode != GUARD_BODY) continue;
        if (s->guards[i].x == x && s->guards[i].y == y) return i;
    }
    return 0xFF;
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

/* Direction to look at (tx,ty): along whichever axis is further away
 * (horizontal on a tie). */
static uint8_t dir_look(uint8_t x, uint8_t y, uint8_t tx, uint8_t ty)
{
    uint8_t dx = x > tx ? x - tx : tx - x;
    uint8_t dy = y > ty ? y - ty : ty - y;
    if (dx >= dy) return tx > x ? DIR_RIGHT : DIR_LEFT;
    return ty > y ? DIR_DOWN : DIR_UP;
}

#define STEP_OK     0
#define STEP_BUMP   1   /* walked into the player */
#define STEP_NOPATH 2
#define STEP_BESIDE 3   /* target is a tile guards can't enter; now next to it */

/* Move guard one tile along a shortest path to (x, y). */
static uint8_t step_toward(GameState *s, uint8_t idx, uint8_t x, uint8_t y)
{
    Guard  *g = &s->guards[idx];
    uint8_t d, here, nx, ny;

    distances_to(x, y);
    here = DIST(g->x, g->y);
    if (here == NO_PATH) return STEP_NOPATH;
    if (here == 0) return STEP_OK;

    /* first neighbour (up, right, down, left) that is one step closer */
    for (d = 0; d < 4; d++) {
        nx = g->x + dir_dx[d];
        ny = g->y + dir_dy[d];
        if (nx >= map_w || ny >= map_h) continue;   /* off the map */
        if (DIST(nx, ny) == here - 1) break;
    }
    if (d == 4) return STEP_NOPATH;   /* path too long to track: stay put */

    g->facing = d;
    if (game_blocks(nx, ny, BLOCK_GUARD)) return STEP_BESIDE;   /* e.g. railing */
    if (nx == s->px && ny == s->py) {
        if (game_attitude(s, idx) == ATT_BLIND) return STEP_OK;  /* waits */
        return STEP_BUMP;
    }
    if (guard_at(s, nx, ny, idx)) return STEP_OK;   /* blocked: wait */
    g->x = nx;
    g->y = ny;
    return STEP_OK;
}

/* Normal routine: walk the patrol loop, or stand at post. */
static uint8_t guard_patrol(GameState *s, uint8_t idx)
{
    Guard          *g = &s->guards[idx];
    const GuardDef *d = &level->guards[idx];
    uint8_t         dir, nx, ny;
    const Point    *target;

    if (d->wp_count < 2) {          /* stationary sentry */
        g->facing = d->facing;      /* back to watching its post */
        return 0;
    }

    target = &d->wp[g->next_wp];
    dir    = dir_toward(g->x, g->y, target);
    nx     = g->x + dir_dx[dir];
    ny     = g->y + dir_dy[dir];
    g->facing = dir;

    if (game_blocks(nx, ny, BLOCK_GUARD)) {
        /* The straight line to the waypoint is blocked (levels from the
         * editor never do this): find a way round rather than wait forever. */
        if (step_toward(s, idx, target->x, target->y) == STEP_BUMP) return 1;
    } else {
        if (nx == s->px && ny == s->py)
            return game_attitude(s, idx) != ATT_BLIND;  /* blind guards wait */
        if (guard_at(s, nx, ny, idx)) return 0;   /* blocked: wait this turn */
        g->x = nx;
        g->y = ny;
    }

    /* Reached the waypoint: pick the next one and turn to face it, so the
     * cone always shows where the guard is about to walk. */
    if (g->x == target->x && g->y == target->y) {
        g->next_wp++;
        if (g->next_wp >= d->wp_count) g->next_wp = 0;
        g->facing = dir_toward(g->x, g->y, &d->wp[g->next_wp]);
    }
    return 0;
}

/* Called when a guard gets back to the spot where it left its route. */
static void guard_resume(GameState *s, uint8_t idx)
{
    Guard          *g = &s->guards[idx];
    const GuardDef *d = &level->guards[idx];

    g->mode = GUARD_PATROL;
    g->hx = g->hy = 0;   /* unused while patrolling; cleared so identical */
    g->tx = g->ty = 0;   /* situations are byte-identical states */
    if (d->wp_count < 2) g->facing = d->facing;
    else g->facing = dir_toward(g->x, g->y, &d->wp[g->next_wp]);
}

/* One guard's turn. Returns 1 if it bumped into the player. */
static uint8_t guard_act(GameState *s, uint8_t idx)
{
    Guard  *g = &s->guards[idx];
    uint8_t r;

    if (g->mode == GUARD_INVESTIGATE) {
        if (g->x == g->tx && g->y == g->ty) {
            g->mode = GUARD_RETURN;     /* nothing here; head back */
            g->tx = g->ty = 0;
        } else {
            r = step_toward(s, idx, g->tx, g->ty);
            if (r == STEP_BUMP) return 1;
            if (r == STEP_NOPATH || r == STEP_BESIDE) {   /* as close as it gets */
                g->mode = GUARD_RETURN;
                g->tx = g->ty = 0;
            }
            return 0;
        }
    }

    if (g->mode == GUARD_RETURN) {
        if (g->x == g->hx && g->y == g->hy) {
            guard_resume(s, idx);       /* already home: patrol this turn */
        } else {
            r = step_toward(s, idx, g->hx, g->hy);
            if (r == STEP_BUMP) return 1;
            if (g->x == g->hx && g->y == g->hy) guard_resume(s, idx);
            return 0;
        }
    }

    return guard_patrol(s, idx);
}

/* Everyone within earshot of (x, y) turns to look; the closest by walking
 * distance is sent to investigate. Returns a bitmask of guards that heard
 * it (they spend this turn looking, so they don't otherwise act). */
static uint8_t react_to_noise(GameState *s, uint8_t x, uint8_t y, uint8_t radius)
{
    uint8_t i, heard = 0, best = 0xFF, best_dist = NO_PATH, d;
    Guard  *g;

    distances_to(x, y);

    for (i = 0; i < s->guard_count; i++) {
        g = &s->guards[i];
        if (!GUARD_ACTIVE(g)) continue;
        if (game_distance(g->x, g->y, x, y) > radius) continue;
        heard |= (uint8_t)(1 << i);
        if (g->x != x || g->y != y) g->facing = dir_look(g->x, g->y, x, y);
        d = DIST(g->x, g->y);
        if (d < best_dist) {        /* ties go to the lower guard index */
            best_dist = d;
            best = i;
        }
    }

    if (best != 0xFF) {
        g = &s->guards[best];
        if (g->mode == GUARD_PATROL) {  /* remember where to come back to */
            g->hx = g->x;
            g->hy = g->y;
        }
        g->mode = GUARD_INVESTIGATE;
        g->tx   = x;
        g->ty   = y;
    }
    return heard;
}

/* ------------------------------------------------------------------ */
/* Turn                                                                */
/* ------------------------------------------------------------------ */

/* Caught or alarm? Sets status and returns 1 if the level is lost. */
static uint8_t check_detection(GameState *s, uint8_t suspicious)
{
    if (player_spotted(s, suspicious)) {
        s->status = STATUS_CAUGHT;
        return 1;
    }
    if (body_spotted(s)) {
        s->status = STATUS_ALARM;
        return 1;
    }
    return 0;
}

/* Everything after the player's action. noise = 0 means nothing thrown;
 * suspicious = the action was one that blind guards notice too. */
static void finish_turn(GameState *s, uint8_t noise, uint8_t nx, uint8_t ny,
                        uint8_t suspicious)
{
    uint8_t i, heard = 0;

    s->turn++;

    if (game_cell(s->px, s->py) == CELL_EXIT) {
        s->status = STATUS_WON;
        return;
    }
    if (check_detection(s, suspicious)) return;

    if (noise) heard = react_to_noise(s, nx, ny, noise);

    for (i = 0; i < s->guard_count; i++) {
        if (heard & (1 << i)) continue;
        if (!GUARD_ACTIVE(&s->guards[i])) continue;
        if (guard_act(s, i)) {
            s->status = STATUS_CAUGHT;
            return;
        }
    }

    if (check_detection(s, suspicious)) return;

    if (s->takedown_left) {
        s->takedown_left--;
        if (s->takedown_left == 0) {
            Guard *g = &s->guards[s->takedown_guard];
            g->mode = GUARD_BODY;
            g->tx = g->ty = g->hx = g->hy = 0;
            s->takedown_guard = 0;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Talking and scripts                                                 */
/* ------------------------------------------------------------------ */

uint8_t game_talk_target(const GameState *s)
{
    uint8_t i;
    if (s->status != STATUS_PLAYING || s->takedown_left) return 0xFF;
    for (i = 0; i < level->npc_count; i++) {
        const NpcDef *n = &level->npcs[i];
        if (n->script && game_distance(n->x, n->y, s->px, s->py) == 1) return i;
    }
    return 0xFF;
}

uint8_t game_said_count(void)    { return said_n; }
uint8_t game_said(uint8_t i)     { return said[i]; }

/* Run a script (see the OP_* list in game.h). */
static void run_script(GameState *s, const uint8_t *p)
{
    for (;;) {
        switch (*p++) {
        case OP_SAY:
            if (said_n < MAX_SAID) said[said_n++] = *p;
            p++;
            break;
        case OP_SET:
            s->flags |= *p++;
            break;
        case OP_CLEAR:
            s->flags &= (uint8_t)~*p++;
            break;
        case OP_IF:
            if (!(s->flags & p[0])) p += p[1];
            p += 2;
            break;
        case OP_JUMP:
            p += p[0] + 1;
            break;
        default:                        /* OP_END */
            game_set_flags(s->flags);   /* switch cells follow at once */
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Takedowns and bodies                                                */
/* ------------------------------------------------------------------ */

uint8_t game_takedown_target(const GameState *s)
{
    uint8_t i;
    if (s->status != STATUS_PLAYING || s->takedown_left) return 0xFF;
    for (i = 0; i < s->guard_count; i++) {
        const Guard *g = &s->guards[i];
        if (!GUARD_ACTIVE(g)) continue;
        if (game_distance(g->x, g->y, s->px, s->py) == 1) return i;
    }
    return 0xFF;
}

uint8_t game_uniform_here(const GameState *s)
{
    uint8_t b;
    if (s->status != STATUS_PLAYING || s->takedown_left) return 0xFF;
    b = body_at(s, s->px, s->py);
    if (b == 0xFF || s->guards[b].stripped) return 0xFF;
    if (level->guards[b].uniform == DISGUISE_NONE) return 0xFF;
    return b;
}

uint8_t game_body_adjacent(const GameState *s)
{
    uint8_t d;
    for (d = 0; d < 4; d++)
        if (body_at(s, s->px + dir_dx[d], s->py + dir_dy[d]) != 0xFF) return 1;
    return 0;
}

/* Can something (player or pushed body) be moved onto this tile? */
static uint8_t tile_free(const GameState *s, uint8_t x, uint8_t y)
{
    return !game_blocks(x, y, BLOCK_PLAYER) && !guard_at(s, x, y, 0xFF);
}

/* Work out a drag/push in direction dir. Returns the body index (0xFF if
 * illegal) and sets *bx,*by to where the body ends up. */
static uint8_t plan_drag(const GameState *s, uint8_t dir, uint8_t *bx, uint8_t *by)
{
    uint8_t nx = s->px + dir_dx[dir];
    uint8_t ny = s->py + dir_dy[dir];
    uint8_t b, d;

    if (s->status != STATUS_PLAYING || s->takedown_left) return 0xFF;

    /* either way the player steps to (nx,ny): it must be enterable (a body
     * can lie where the player can't go, e.g. a guard taken down on a
     * security door) */
    if (!tile_free(s, nx, ny)) return 0xFF;

    /* push: body straight ahead moves one further */
    b = body_at(s, nx, ny);
    if (b != 0xFF) {
        *bx = nx + dir_dx[dir];
        *by = ny + dir_dy[dir];
        if (!tile_free(s, *bx, *by) || body_at(s, *bx, *by) != 0xFF) return 0xFF;
        return b;
    }

    /* drag: an adjacent body follows into the tile we leave. Prefer the
     * one directly behind, then up/right/down/left. Not while standing on
     * a body (the two would end up stacked). */
    if (body_at(s, s->px, s->py) != 0xFF) return 0xFF;
    b = body_at(s, s->px - dir_dx[dir], s->py - dir_dy[dir]);
    for (d = 0; b == 0xFF && d < 4; d++)
        b = body_at(s, s->px + dir_dx[d], s->py + dir_dy[d]);
    if (b == 0xFF) return 0xFF;
    *bx = s->px;
    *by = s->py;
    return b;
}

uint8_t game_can_drag(const GameState *s, uint8_t dir)
{
    uint8_t bx, by;
    return plan_drag(s, dir, &bx, &by) != 0xFF;
}

/* ------------------------------------------------------------------ */
/* Player actions                                                      */
/* ------------------------------------------------------------------ */

uint8_t game_step(GameState *s, uint8_t action)
{
    uint8_t nx, ny, b, bx, by;
    uint8_t suspicious = s->takedown_left != 0;   /* mid-takedown */

    said_n = 0;
    game_set_flags(s->flags);
    if (s->status != STATUS_PLAYING) return 0;
    if (s->takedown_left && action != ACT_WAIT) return 0;  /* locked */

    if (action < 4) {
        nx = s->px + dir_dx[action];
        ny = s->py + dir_dy[action];
        if (!tile_free(s, nx, ny)) return 0;   /* walls and standing guards */
        s->px = nx;
        s->py = ny;
    } else if (action >= ACT_DRAG && action < ACT_DRAG + 4) {
        uint8_t dir = action - ACT_DRAG;
        b = plan_drag(s, dir, &bx, &by);
        if (b == 0xFF) return 0;
        s->px += dir_dx[dir];
        s->py += dir_dy[dir];
        s->guards[b].x = bx;
        s->guards[b].y = by;
        suspicious = 1;
    } else if (action == ACT_UNIFORM) {
        b = game_uniform_here(s);
        if (b == 0xFF) return 0;
        s->disguise = level->guards[b].uniform;
        s->guards[b].stripped = 1;
        suspicious = 1;
    } else if (action == ACT_TAKEDOWN) {
        b = game_takedown_target(s);
        if (b == 0xFF) return 0;
        s->guards[b].mode  = GUARD_SUBDUED;
        s->takedown_guard  = b;
        s->takedown_left   = TAKEDOWN_TURNS;
        suspicious = 1;
    } else if (action == ACT_TALK) {
        b = game_talk_target(s);
        if (b == 0xFF) return 0;
        run_script(s, level->npcs[b].script);
    } else if (action != ACT_WAIT) {
        return 0;
    }

    finish_turn(s, 0, 0, 0, suspicious);
    return 1;
}

uint8_t game_can_throw(const GameState *s, uint8_t item, uint8_t x, uint8_t y)
{
    uint8_t d;

    if (item >= ITEM_TYPES || s->items[item] == 0) return 0;
    /* cheapest test first: aiming asks about every tile on screen */
    d = game_distance(s->px, s->py, x, y);
    if (d == 0 || d > item_defs[item].range) return 0;
    if (game_blocks(x, y, BLOCK_THROW)) return 0;   /* also bounds-checks */
    return line_of_sight((int8_t)s->px, (int8_t)s->py, (int8_t)x, (int8_t)y, BLOCK_THROW);
}

uint8_t game_throw(GameState *s, uint8_t item, uint8_t x, uint8_t y)
{
    said_n = 0;
    game_set_flags(s->flags);
    if (s->status != STATUS_PLAYING || s->takedown_left) return 0;
    if (!game_can_throw(s, item, x, y)) return 0;

    s->items[item]--;
    finish_turn(s, item_defs[item].noise, x, y, 1);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Arriving from another map (see world.c)                             */
/* ------------------------------------------------------------------ */

uint8_t game_guard_at(const GameState *s, uint8_t x, uint8_t y)
{
    return guard_at(s, x, y, 0xFF);
}

uint8_t game_arrive(GameState *s, uint8_t x, uint8_t y)
{
    said_n = 0;
    game_set_flags(s->flags);
    s->px     = x;
    s->py     = y;
    s->status = STATUS_PLAYING;
    s->turn++;
    if (game_cell(x, y) == CELL_EXIT) s->status = STATUS_WON;
    else check_detection(s, 0);
    return 1;
}
