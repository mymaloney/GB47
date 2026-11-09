/*
 * world.c - Levels made of several linked maps. See world.h.
 */
#include <string.h>
#include "world.h"

static const LevelDef *table;     /* the level table world_load was given */

static const int8_t dir_dx[4] = { 0, 1, 0, -1 };
static const int8_t dir_dy[4] = { -1, 0, 1, 0 };

/* The player isn't in this map: clear the fields that only mean something
 * where the player is, so equal situations stay byte-for-byte equal. */
static void blank_player(GameState *s)
{
    uint8_t k;
    for (k = 0; k < ITEM_TYPES; k++) s->items[k] = 0;
    s->disguise = 0;
    s->turn     = 0;
    s->px = s->py = 0;
    s->status = STATUS_PLAYING;
}

void world_load(const LevelDef *tbl, uint8_t first, World *w)
{
    uint8_t i, d, t, k, found;

    table = tbl;
    memset(w, 0, sizeof(World));
    w->part_def[0] = first;
    w->part_count  = 1;

    /* every map reachable through links belongs to this level */
    for (i = 0; i < w->part_count; i++) {
        const LevelDef *def = &table[w->part_def[i]];
        for (d = 0; d < 4; d++) {
            if (!def->link[d]) continue;
            t = def->link[d] - 1;
            found = 0;
            for (k = 0; k < w->part_count; k++)
                if (w->part_def[k] == t) found = 1;
            if (!found && w->part_count < MAX_PARTS)
                w->part_def[w->part_count++] = t;
        }
    }

    /* fresh state for every map; the player starts in the first */
    for (i = w->part_count; i-- > 0; )
        game_load(&table[w->part_def[i]], &w->part[i]);
    for (i = 1; i < w->part_count; i++) blank_player(&w->part[i]);
    w->cur = 0;   /* game_load(part 0) ran last, so it's the current map */
}

void world_activate(const World *w)
{
    game_use(&table[w->part_def[w->cur]]);
}

GameState *world_state(World *w)            { return &w->part[w->cur]; }
const LevelDef *world_def(const World *w)   { return &table[w->part_def[w->cur]]; }

/* Try to step off the edge in direction dir. Returns 1 if the player
 * crossed into the linked map. */
static uint8_t cross(World *w, uint8_t dir)
{
    GameState      *s   = &w->part[w->cur];
    const LevelDef *def = world_def(w);
    const LevelDef *nd;
    GameState      *n;
    uint8_t         to, p, k;
    int16_t         tx, ty;

    if (!def->link[dir]) return 0;
    to = def->link[dir] - 1;
    for (p = 0; p < w->part_count && w->part_def[p] != to; p++) {}
    if (p == w->part_count) return 0;
    nd = &table[to];

    /* where along the neighbour's facing edge the player comes in */
    switch (dir) {
    case DIR_RIGHT: tx = 0;              ty = (int16_t)s->py - def->link_offset[dir]; break;
    case DIR_LEFT:  tx = nd->width - 1;  ty = (int16_t)s->py - def->link_offset[dir]; break;
    case DIR_DOWN:  ty = 0;              tx = (int16_t)s->px - def->link_offset[dir]; break;
    default:        ty = nd->height - 1; tx = (int16_t)s->px - def->link_offset[dir]; break;
    }
    if (tx < 0 || ty < 0 || tx >= nd->width || ty >= nd->height) return 0;

    n = &w->part[p];
    game_use(nd);
    if (game_blocks((uint8_t)tx, (uint8_t)ty, BLOCK_PLAYER) ||
        game_guard_at(n, (uint8_t)tx, (uint8_t)ty)) {
        game_use(def);
        return 0;
    }

    /* carry the player's things over, then blank the map being left */
    for (k = 0; k < ITEM_TYPES; k++) n->items[k] = s->items[k];
    n->disguise = s->disguise;
    n->turn     = s->turn;
    blank_player(s);

    w->cur = p;
    return game_arrive(n, (uint8_t)tx, (uint8_t)ty);
}

uint8_t world_step(World *w, uint8_t action)
{
    GameState *s = &w->part[w->cur];

    world_activate(w);
    if (action < 4 && s->status == STATUS_PLAYING && !s->takedown_left) {
        uint8_t nx = s->px + dir_dx[action];
        uint8_t ny = s->py + dir_dy[action];
        if (nx >= world_def(w)->width || ny >= world_def(w)->height)
            return cross(w, action);
    }
    return game_step(s, action);
}

uint8_t world_throw(World *w, uint8_t item, uint8_t x, uint8_t y)
{
    world_activate(w);
    return game_throw(&w->part[w->cur], item, x, y);
}

/* ------------------------------------------------------------------ */
/* Undo                                                                */
/* ------------------------------------------------------------------ */

/* Restoring the map that was current before a crossing is enough: the
 * crossing changed nothing in the entered map except the player's own
 * fields, which undo blanks again; turns taken in that map are undone
 * first. */
typedef struct {
    uint8_t   cur;
    GameState s;
} UndoEntry;

static UndoEntry history[UNDO_DEPTH];
static uint8_t   hist_top;    /* next free slot */
static uint8_t   hist_count;

void undo_clear(void)
{
    hist_top   = 0;
    hist_count = 0;
}

void undo_push(const World *w)
{
    history[hist_top].cur = w->cur;
    history[hist_top].s   = w->part[w->cur];
    hist_top = (hist_top + 1) % UNDO_DEPTH;
    if (hist_count < UNDO_DEPTH) hist_count++;
}

uint8_t undo_pop(World *w)
{
    if (hist_count == 0) return 0;
    hist_top = (hist_top + UNDO_DEPTH - 1) % UNDO_DEPTH;
    hist_count--;
    if (w->cur != history[hist_top].cur)      /* undoing a crossing */
        blank_player(&w->part[w->cur]);
    w->cur = history[hist_top].cur;
    w->part[w->cur] = history[hist_top].s;
    world_activate(w);
    return 1;
}
