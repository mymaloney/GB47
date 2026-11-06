/*
 * solver.c - Breadth-first search over every reachable game state.
 * See solver.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "solver.h"

#define MAX_STATES (1 << 20)
#define HASH_SIZE  (1 << 22)

typedef struct {
    GameState s;
    int       parent;
    SolveStep step;
} Node;

static Node *nodes;
static int  *table;        /* open addressing: node index + 1, 0 = empty */

static uint32_t hash_state(const GameState *s)
{
    const uint8_t *p = (const uint8_t *)s;
    uint32_t h = 2166136261u;
    size_t i;
    for (i = 0; i < sizeof *s; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

/* Returns 1 if newly added. */
static int visit(const GameState *s, int count)
{
    uint32_t h = hash_state(s) & (HASH_SIZE - 1);
    while (table[h]) {
        if (memcmp(&nodes[table[h] - 1].s, s, sizeof *s) == 0) return 0;
        h = (h + 1) & (HASH_SIZE - 1);
    }
    table[h] = count + 1;
    return 1;
}

int solver_run(const LevelDef *lv, int use, SolveStep *steps, int max_steps,
               int *states)
{
    int head = 0, tail = 1, found = -1, i;

    if (!nodes) nodes = malloc(sizeof(Node) * MAX_STATES);
    if (!table) table = malloc(sizeof(int) * HASH_SIZE);
    if (!nodes || !table) return -1;
    memset(table, 0, sizeof(int) * HASH_SIZE);

    game_load(lv, &nodes[0].s);
    if (!(use & SOLVE_USE_ITEMS))
        for (i = 0; i < ITEM_TYPES; i++) nodes[0].s.items[i] = 0;
    nodes[0].parent = -1;
    visit(&nodes[0].s, 0);

    while (head < tail && found < 0) {
        int cur = head++;
        const GameState *cs = &nodes[cur].s;
        int a, item, dx, dy, ncand = 0;
        SolveStep cand[ACT_COUNT + ITEM_TYPES * 13 * 13];

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
                    int x = cs->px + dx, y = cs->py + dy;
                    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) continue;
                    if (!game_can_throw(cs, (uint8_t)item, (uint8_t)x, (uint8_t)y)) continue;
                    cand[ncand].action = (uint8_t)(SOLVE_THROW | item);
                    cand[ncand].tx = (uint8_t)x;
                    cand[ncand].ty = (uint8_t)y;
                    ncand++;
                }
        }

        for (a = 0; a < ncand && found < 0; a++) {
            GameState n = *cs;
            const SolveStep *c = &cand[a];
            if (c->action & SOLVE_THROW) {
                if (!game_throw(&n, c->action & 0x7F, c->tx, c->ty)) continue;
            } else if (!game_step(&n, c->action)) {
                continue;
            }
            if (n.status == STATUS_CAUGHT || n.status == STATUS_ALARM) continue;
            n.turn = 0;   /* same situation at a different turn = same state */
            if (tail >= MAX_STATES) {
                if (states) *states = tail;
                return -1;
            }
            if (!visit(&n, tail)) continue;
            nodes[tail].s = n;
            nodes[tail].parent = cur;
            nodes[tail].step = *c;
            if (n.status == STATUS_WON) found = tail;
            tail++;
        }
    }
    if (states) *states = tail;
    if (found < 0) return 0;

    {
        int len = 0, k;
        for (k = found; nodes[k].parent >= 0; k = nodes[k].parent) len++;
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
