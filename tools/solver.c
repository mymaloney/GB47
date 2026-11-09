/*
 * solver.c - Breadth-first search over every reachable game state.
 * See solver.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "solver.h"

#define MAX_STATES  (1 << 21)   /* distinct situations kept (~60 bytes each) */
#define HASH_SIZE   (1 << 23)
#define MAX_ENTRIES (1 << 22)   /* queue entries, including re-queued ones */
#define MAX_F       4096        /* turns + distance-to-exit, upper bound */
#define MAX_EXITS   64

typedef struct {
    GameState s;
    int       parent;
    uint16_t  g;           /* turns from the start */
    SolveStep step;
} Node;

static Node *nodes;
static int  *table;        /* open addressing: node index + 1, 0 = empty */
static int  *entry_node, *entry_next;   /* bucket queue, one list per f */
static int   bucket_head[MAX_F], bucket_tail[MAX_F];
static int   entries;
static uint8_t exit_x[MAX_EXITS], exit_y[MAX_EXITS];
static int   exit_count;

static uint32_t hash_state(const GameState *s)
{
    const uint8_t *p = (const uint8_t *)s;
    uint32_t h = 2166136261u;
    size_t i;
    for (i = 0; i < sizeof *s; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

/* Index of the node holding this state, or -1 (and *slot = where to add). */
static int find(const GameState *s, uint32_t *slot)
{
    uint32_t h = hash_state(s) & (HASH_SIZE - 1);
    while (table[h]) {
        if (memcmp(&nodes[table[h] - 1].s, s, sizeof *s) == 0) return table[h] - 1;
        h = (h + 1) & (HASH_SIZE - 1);
    }
    *slot = h;
    return -1;
}

/* Lower bound on turns left: walking distance to the nearest exit,
 * ignoring walls (the player moves at most one tile per turn). */
static int heuristic(const GameState *s)
{
    int i, best = 0x7FFF;
    for (i = 0; i < exit_count; i++) {
        int d = game_distance(s->px, s->py, exit_x[i], exit_y[i]);
        if (d < best) best = d;
    }
    return exit_count ? best : 0;
}

static int push(int node, int f)
{
    if (entries >= MAX_ENTRIES || f >= MAX_F) return 0;
    entry_node[entries] = node;
    entry_next[entries] = -1;
    if (bucket_head[f] < 0) bucket_head[f] = entries;
    else entry_next[bucket_tail[f]] = entries;
    bucket_tail[f] = entries;
    entries++;
    return 1;
}

/* A* search: shortest solution first. With a heuristic that never
 * overestimates and one turn per action, the first winning situation taken
 * off the queue is a shortest solution. A situation reached again by a
 * shorter route is updated and re-queued. Proving a level unsolvable
 * still means visiting every reachable situation. */
int solver_run(const LevelDef *lv, int use, SolveStep *steps, int max_steps,
               int *states)
{
    int tail = 1, found = -1, i, f, x, y, overflow = 0;
    uint32_t slot;

    if (!nodes) nodes = malloc(sizeof(Node) * MAX_STATES);
    if (!table) table = malloc(sizeof(int) * HASH_SIZE);
    if (!entry_node) entry_node = malloc(sizeof(int) * MAX_ENTRIES);
    if (!entry_next) entry_next = malloc(sizeof(int) * MAX_ENTRIES);
    if (!nodes || !table || !entry_node || !entry_next) return -1;
    memset(table, 0, sizeof(int) * HASH_SIZE);
    for (f = 0; f < MAX_F; f++) bucket_head[f] = bucket_tail[f] = -1;
    entries = 0;

    game_load(lv, &nodes[0].s);
    if (!(use & SOLVE_USE_ITEMS))
        for (i = 0; i < ITEM_TYPES; i++) nodes[0].s.items[i] = 0;
    exit_count = 0;
    for (y = 0; y < game_height(); y++)
        for (x = 0; x < game_width(); x++)
            if (game_cell((uint8_t)x, (uint8_t)y) == CELL_EXIT && exit_count < MAX_EXITS) {
                exit_x[exit_count] = (uint8_t)x;
                exit_y[exit_count] = (uint8_t)y;
                exit_count++;
            }
    nodes[0].parent = -1;
    nodes[0].g = 0;
    find(&nodes[0].s, &slot);
    table[slot] = 1;
    push(0, heuristic(&nodes[0].s));

    for (f = 0; f < MAX_F && found < 0; f++) {
        int e;
        /* entries can be appended to this bucket while it is processed */
        for (e = bucket_head[f]; e >= 0 && found < 0; e = entry_next[e]) {
            int cur = entry_node[e];
            const GameState *cs = &nodes[cur].s;
            int a, item, dx, dy, ncand = 0;
            SolveStep cand[ACT_COUNT + ITEM_TYPES * 13 * 13];

            if (nodes[cur].g + heuristic(cs) != f) continue;   /* stale entry */
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
                GameState n = *cs;
                const SolveStep *c = &cand[a];
                int g = nodes[cur].g + 1, k;
                if (c->action & SOLVE_THROW) {
                    if (!game_throw(&n, c->action & 0x7F, c->tx, c->ty)) continue;
                } else if (!game_step(&n, c->action)) {
                    continue;
                }
                if (n.status == STATUS_CAUGHT || n.status == STATUS_ALARM) continue;
                n.turn = 0;   /* same situation at a different turn = same state */
                k = find(&n, &slot);
                if (k < 0) {
                    if (tail >= MAX_STATES) { overflow = 1; continue; }
                    k = tail++;
                    table[slot] = k + 1;
                    nodes[k].s = n;
                } else if (g >= nodes[k].g) {
                    continue;          /* already reached at least as fast */
                }
                nodes[k].parent = cur;
                nodes[k].g = (uint16_t)g;
                nodes[k].step = *c;
                if (!push(k, g + heuristic(&n))) overflow = 1;
            }
        }
    }
    if (states) *states = tail;
    if (found < 0) return overflow ? -1 : 0;

    {
        int len = nodes[found].g, k;
        if (steps) {
            i = len;
            for (k = found; nodes[k].parent >= 0; k = nodes[k].parent) {
                i--;
                if (i < max_steps) steps[i] = nodes[k].step;
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
