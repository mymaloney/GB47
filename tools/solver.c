/*
 * solver.c - A* search over every reachable situation of a level (all of
 * its maps). See solver.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "solver.h"

#define MEMORY_BUDGET ((size_t)192 << 20)   /* bytes for stored situations */
#define MAX_STATES  (1 << 21)
#define HASH_SIZE   (1 << 23)
#define MAX_ENTRIES (1 << 22)   /* queue entries, including re-queued ones */
#define MAX_F       4096        /* turns + distance estimate, upper bound */
#define MAX_EXITS   64

/* Situations are stored as a header plus only the bytes of the World that
 * this level uses, so one-map levels cost no more than before. */
typedef struct {
    int       parent;
    uint16_t  g;           /* turns from the start */
    uint16_t  queued_f;    /* bucket it was last queued in */
    SolveStep step;
} Head;

static uint8_t *pool;
static size_t   stride, world_bytes, max_states;
static int     *table_;    /* open addressing: node index + 1, 0 = empty */
static int     *entry_node, *entry_next;
static int      bucket_head[MAX_F], bucket_tail[MAX_F];
static int      entries;

static uint8_t exit_part[MAX_EXITS], exit_x[MAX_EXITS], exit_y[MAX_EXITS];
static int     exit_count;
/* beyond[p][dir]: fewest steps from anywhere on the edge you enter by
 * when leaving map p through dir, to an exit in that next map (0 if it
 * has none: the exit may be further on). */
static int     beyond[MAX_PARTS][4];

#define HEAD(i)  ((Head *)(pool + (size_t)(i) * stride))
#define WORLD(i) ((World *)(pool + (size_t)(i) * stride + sizeof(Head)))

static uint32_t hash_bytes(const void *p_, size_t n)
{
    const uint8_t *p = (const uint8_t *)p_;
    uint32_t h = 2166136261u;
    size_t i;
    for (i = 0; i < n; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

/* Index of the node holding this world, or -1 (and *slot = where to add). */
static int find(const World *w, uint32_t *slot)
{
    uint32_t h = hash_bytes(w, world_bytes) & (HASH_SIZE - 1);
    while (table_[h]) {
        if (memcmp(WORLD(table_[h] - 1), w, world_bytes) == 0) return table_[h] - 1;
        h = (h + 1) & (HASH_SIZE - 1);
    }
    *slot = h;
    return -1;
}

/* Lower bound on turns left, ignoring walls: walk to the nearest exit in
 * this map, or to a linked edge, cross, and walk on to an exit there. */
static int heuristic(const World *w)
{
    const GameState *s   = &w->part[w->cur];
    const LevelDef  *def = world_def(w);
    const int       *on  = beyond[w->cur];
    int i, d, best = 0x7FFF;
    for (i = 0; i < exit_count; i++) {
        if (exit_part[i] != w->cur) continue;
        d = game_distance(s->px, s->py, exit_x[i], exit_y[i]);
        if (d < best) best = d;
    }
    if (def->link[DIR_UP]    && s->py + 1 + on[DIR_UP] < best)     best = s->py + 1 + on[DIR_UP];
    if (def->link[DIR_LEFT]  && s->px + 1 + on[DIR_LEFT] < best)   best = s->px + 1 + on[DIR_LEFT];
    if (def->link[DIR_DOWN]  && def->height - s->py + on[DIR_DOWN] < best)
        best = def->height - s->py + on[DIR_DOWN];
    if (def->link[DIR_RIGHT] && def->width - s->px + on[DIR_RIGHT] < best)
        best = def->width - s->px + on[DIR_RIGHT];
    return best == 0x7FFF ? 0 : best;
}

/* Fill beyond[][] (see above) once the exits are known. */
static void compute_beyond(const LevelDef *tbl, const World *w)
{
    int p, d, q, k, i, x, y, best, any;
    for (p = 0; p < w->part_count; p++)
        for (d = 0; d < 4; d++) {
            const LevelDef *def = &tbl[w->part_def[p]];
            const LevelDef *nd;
            beyond[p][d] = 0;
            if (!def->link[d]) continue;
            for (q = 0; q < w->part_count && w->part_def[q] != def->link[d] - 1; q++) {}
            if (q == w->part_count) continue;
            nd = &tbl[w->part_def[q]];
            best = 0x7FFF;
            any = 0;
            /* every tile on the edge of map q that you'd arrive on */
            for (k = 0; k < ((d == DIR_LEFT || d == DIR_RIGHT) ? nd->height : nd->width); k++) {
                x = d == DIR_RIGHT ? 0 : d == DIR_LEFT ? nd->width - 1 : k;
                y = d == DIR_DOWN ? 0 : d == DIR_UP ? nd->height - 1 : k;
                for (i = 0; i < exit_count; i++) {
                    if (exit_part[i] != q) continue;
                    any = 1;
                    if (game_distance((uint8_t)x, (uint8_t)y, exit_x[i], exit_y[i]) < best)
                        best = game_distance((uint8_t)x, (uint8_t)y, exit_x[i], exit_y[i]);
                }
            }
            beyond[p][d] = any ? best : 0;
        }
}

static int push(int node, int f)
{
    if (entries >= MAX_ENTRIES || f >= MAX_F) return 0;
    HEAD(node)->queued_f = (uint16_t)f;
    entry_node[entries] = node;
    entry_next[entries] = -1;
    if (bucket_head[f] < 0) bucket_head[f] = entries;
    else entry_next[bucket_tail[f]] = entries;
    bucket_tail[f] = entries;
    entries++;
    return 1;
}

/* A* search: with a heuristic that never overestimates and one turn per
 * action, the first winning situation taken off the queue is a shortest
 * solution. A situation reached again by a shorter route is updated and
 * re-queued. Proving a level unsolvable means visiting everything. */
int solver_run(const LevelDef *tbl, uint8_t first, int use, SolveStep *steps,
               int max_steps, int *states)
{
    World    start;
    int      tail = 1, found = -1, i, f, x, y, p, overflow = 0;
    uint32_t slot;

    world_load(tbl, first, &start);
    if (!(use & SOLVE_USE_ITEMS))
        for (i = 0; i < ITEM_TYPES; i++) start.part[0].items[i] = 0;

    world_bytes = WORLD_BYTES(&start);
    stride = (sizeof(Head) + world_bytes + 7) & ~(size_t)7;
    max_states = MEMORY_BUDGET / stride;
    if (max_states > MAX_STATES) max_states = MAX_STATES;

    if (!pool) pool = malloc(MEMORY_BUDGET);
    if (!table_) table_ = malloc(sizeof(int) * HASH_SIZE);
    if (!entry_node) entry_node = malloc(sizeof(int) * MAX_ENTRIES);
    if (!entry_next) entry_next = malloc(sizeof(int) * MAX_ENTRIES);
    if (!pool || !table_ || !entry_node || !entry_next) return -1;
    memset(table_, 0, sizeof(int) * HASH_SIZE);
    for (f = 0; f < MAX_F; f++) bucket_head[f] = bucket_tail[f] = -1;
    entries = 0;

    /* exits in every map of the level */
    exit_count = 0;
    for (p = 0; p < start.part_count; p++) {
        game_use(&tbl[start.part_def[p]]);
        for (y = 0; y < game_height(); y++)
            for (x = 0; x < game_width(); x++)
                if (game_cell((uint8_t)x, (uint8_t)y) == CELL_EXIT && exit_count < MAX_EXITS) {
                    exit_part[exit_count] = (uint8_t)p;
                    exit_x[exit_count] = (uint8_t)x;
                    exit_y[exit_count] = (uint8_t)y;
                    exit_count++;
                }
    }
    compute_beyond(tbl, &start);
    world_activate(&start);

    memcpy(WORLD(0), &start, world_bytes);
    HEAD(0)->parent = -1;
    HEAD(0)->g = 0;
    find(&start, &slot);
    table_[slot] = 1;
    push(0, heuristic(&start));

    for (f = 0; f < MAX_F && found < 0; f++) {
        int e;
        /* entries can be appended to this bucket while it is processed */
        for (e = bucket_head[f]; e >= 0 && found < 0; e = entry_next[e]) {
            int cur = entry_node[e];
            World cw;
            const GameState *cs;
            int a, item, dx, dy, ncand = 0;
            SolveStep cand[ACT_COUNT + ITEM_TYPES * 13 * 13];

            memcpy(&cw, WORLD(cur), world_bytes);
            cs = &cw.part[cw.cur];
            {   /* stale entry: this situation was re-queued with a better g */
                int fc = HEAD(cur)->g + heuristic(&cw);
                if (fc != f && !(fc < f && HEAD(cur)->queued_f == f)) continue;
            }
            if (cs->status == STATUS_WON) { found = cur; break; }

            /* candidate actions: everything in ACT_*, then throws in range */
            for (a = 0; a < ACT_COUNT; a++) {
                if (a >= ACT_DRAG && a < ACT_DRAG + 4 && !(use & SOLVE_USE_DRAG)) continue;
                if (a == ACT_TAKEDOWN && !(use & SOLVE_USE_TAKEDOWN)) continue;
                if (a == ACT_UNIFORM && !(use & SOLVE_USE_UNIFORM)) continue;
                cand[ncand].action = (uint8_t)a;
                cand[ncand].tx = cand[ncand].ty = 0;
                ncand++;
            }
            world_activate(&cw);
            for (item = 0; item < ITEM_TYPES; item++) {
                if (!cs->items[item]) continue;
                for (dy = -6; dy <= 6; dy++)
                    for (dx = -6; dx <= 6; dx++) {
                        int tx = cs->px + dx, ty = cs->py + dy;
                        if (tx < 0 || ty < 0 || tx >= game_width() || ty >= game_height()) continue;
                        if (!game_can_throw(cs, (uint8_t)item, (uint8_t)tx, (uint8_t)ty)) continue;
                        cand[ncand].action = (uint8_t)(SOLVE_THROW | item);
                        cand[ncand].tx = (uint8_t)tx;
                        cand[ncand].ty = (uint8_t)ty;
                        ncand++;
                    }
            }

            for (a = 0; a < ncand; a++) {
                World n;
                const SolveStep *c = &cand[a];
                int g = HEAD(cur)->g + 1, k;
                const GameState *ns;
                memcpy(&n, &cw, world_bytes);
                if (c->action & SOLVE_THROW) {
                    if (!world_throw(&n, c->action & 0x7F, c->tx, c->ty)) continue;
                } else if (!world_step(&n, c->action)) {
                    continue;
                }
                ns = &n.part[n.cur];
                if (ns->status == STATUS_CAUGHT || ns->status == STATUS_ALARM) continue;
                n.part[n.cur].turn = 0;   /* same situation, different turn = same */
                k = find(&n, &slot);
                if (k < 0) {
                    if ((size_t)tail >= max_states) { overflow = 1; continue; }
                    k = tail++;
                    table_[slot] = k + 1;
                    memcpy(WORLD(k), &n, world_bytes);
                } else if (g >= HEAD(k)->g) {
                    continue;          /* already reached at least as fast */
                }
                HEAD(k)->parent = cur;
                HEAD(k)->g = (uint16_t)g;
                HEAD(k)->step = *c;
                {
                    /* "pathmax": never file a situation under a value below
                     * the one being processed. Estimates can drop by more
                     * than a step when crossing between maps, and buckets
                     * below f are already done. */
                    int fk = g + heuristic(&n);
                    if (!push(k, fk < f ? f : fk)) overflow = 1;
                }
            }
        }
    }
    if (states) *states = tail;
    if (found < 0) return overflow ? -1 : 0;

    {
        int len = HEAD(found)->g, k;
        if (steps) {
            i = len;
            for (k = found; HEAD(k)->parent >= 0; k = HEAD(k)->parent) {
                i--;
                if (i < max_steps) steps[i] = HEAD(k)->step;
            }
        }
        return len;
    }
}

void solver_format(const SolveStep *steps, int len, char *out, int outlen)
{
    int k, pos = 0;
    out[0] = 0;
    for (k = 0; k < len && pos < outlen; k++) {
        const SolveStep *st = &steps[k];
        int n;
        if (st->action & SOLVE_THROW)
            n = snprintf(out + pos, outlen - pos, " %c%d,%d ",
                         item_defs[st->action & 0x7F].name[0], st->tx, st->ty);
        else if (st->action >= ACT_DRAG && st->action < ACT_DRAG + 4)
            n = snprintf(out + pos, outlen - pos, "%c", "urdl"[st->action - ACT_DRAG]);
        else if (st->action == ACT_TAKEDOWN)
            n = snprintf(out + pos, outlen - pos, " TAKEDOWN ");
        else if (st->action == ACT_UNIFORM)
            n = snprintf(out + pos, outlen - pos, " UNIFORM ");
        else
            n = snprintf(out + pos, outlen - pos, "%c", "URDLW"[st->action]);
        if (n < 0) break;
        pos += n;
    }
}
