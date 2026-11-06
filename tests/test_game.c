/*
 * test_game.c - Host-side tests for the game rules (compiled with gcc).
 *
 *   make test
 *
 * Besides unit checks, this runs a breadth-first solver over every level to
 * prove it can be completed, and prints the shortest solution. Run it every
 * time you edit a level.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/game.h"
#include "../src/levels.h"

static int failures = 0;

#define CHECK(cond, ...)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            failures++;                                               \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);               \
            printf(__VA_ARGS__);                                      \
            printf("\n");                                             \
        }                                                             \
    } while (0)

/* A tiny level for unit tests. */
static const LevelDef test_level = {
    "TEST",
    {
        "####################",
        "#P.................#",
        "#..................#",
        "#.....#............#",
        "#..................#",
        "#..................#",
        "#.................E#",
        "####################",
        "####################",
        "####################",
        "####################",
        "####################",
        "####################",
        "####################",
        "####################",
        "####################",
    },
    1,
    { { DIR_LEFT, 1, { {10, 3} } } },  /* sentry looking left at the wall */
};

static void test_movement(void)
{
    GameState s;
    game_load(&test_level, &s);
    CHECK(s.px == 1 && s.py == 1, "player start parsed");
    CHECK(game_step(&s, ACT_UP) == 0, "walking into wall is not a turn");
    CHECK(s.turn == 0, "turn not spent");
    CHECK(game_step(&s, ACT_RIGHT) == 1 && s.px == 2, "move right");
    CHECK(game_step(&s, ACT_WAIT) == 1 && s.px == 2 && s.turn == 2, "wait");
}

static void test_vision(void)
{
    GameState s;
    game_load(&test_level, &s);
    const Guard *g = &s.guards[0];
    CHECK(game_guard_sees(g, 9, 3), "sees tile in front");
    CHECK(game_guard_sees(g, 7, 3), "sees 3 ahead");
    CHECK(!game_guard_sees(g, 5, 3), "wall blocks sight");
    CHECK(!game_guard_sees(g, 9, 2), "cone is 1 wide at distance 1");
    CHECK(game_guard_sees(g, 8, 2), "cone is 3 wide at distance 2");
    CHECK(!game_guard_sees(g, 8, 1), "cone is not 5 wide");
    CHECK(!game_guard_sees(g, 11, 3), "cannot see behind");
    CHECK(!game_guard_sees(g, 10, 3), "guard's own tile not in cone");
}

static void test_caught_and_undo(void)
{
    GameState s, before;
    int i;
    game_load(&test_level, &s);
    undo_clear();
    /* walk right along row 2; (6,2) is the far corner of the cone */
    game_step(&s, ACT_DOWN);
    for (i = 0; i < 4; i++) { undo_push(&s); game_step(&s, ACT_RIGHT); }
    CHECK(s.px == 5 && s.status == STATUS_PLAYING, "not yet seen at (5,2)");
    before = s;
    undo_push(&s);
    game_step(&s, ACT_RIGHT);
    CHECK(s.status == STATUS_CAUGHT, "caught entering cone");
    CHECK(game_step(&s, ACT_RIGHT) == 0, "no moves after caught");
    CHECK(undo_pop(&s) == 1, "undo works");
    CHECK(memcmp(&s, &before, sizeof s) == 0, "undo restores state");
}

static void test_patrol(void)
{
    GameState s;
    game_load(&levels[0], &s);
    /* guard 0 of level 1 walks right from (1,5) */
    CHECK(s.guards[0].x == 1, "guard start");
    game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 2 && s.guards[0].facing == DIR_RIGHT, "guard moved");
}

/* Validate level data: row lengths, waypoints on floor and axis aligned. */
static void test_level_data(void)
{
    int l, y, i, w;
    for (l = 0; l < LEVEL_COUNT; l++) {
        const LevelDef *lv = &levels[l];
        int players = 0, exits = 0;
        GameState s;
        for (y = 0; y < MAP_H; y++) {
            CHECK(strlen(lv->rows[y]) == MAP_W, "level %d row %d length %zu",
                  l + 1, y, strlen(lv->rows[y]));
            for (i = 0; i < MAP_W; i++) {
                players += lv->rows[y][i] == 'P';
                exits += lv->rows[y][i] == 'E';
            }
        }
        CHECK(players == 1, "level %d needs exactly one P", l + 1);
        CHECK(exits >= 1, "level %d needs an exit", l + 1);
        CHECK(strlen(lv->name) <= 13, "level %d name too long", l + 1);
        game_load(lv, &s);
        for (i = 0; i < lv->guard_count; i++) {
            const GuardDef *d = &lv->guards[i];
            for (w = 0; w < d->wp_count; w++) {
                Point a = d->wp[w], b = d->wp[(w + 1) % d->wp_count];
                CHECK(game_cell(a.x, a.y) != CELL_WALL,
                      "level %d guard %d waypoint %d in wall", l + 1, i, w);
                CHECK(a.x == b.x || a.y == b.y,
                      "level %d guard %d leg %d not straight", l + 1, i, w);
            }
        }
    }
}

/* ---- Breadth-first solver ---- */

#define MAX_STATES 200000

typedef struct {
    GameState s;
    int       parent;
    uint8_t   action;
} Node;

static int same_state(const GameState *a, const GameState *b)
{
    /* ignore the turn counter so we don't revisit equivalent states */
    GameState x = *a, y = *b;
    x.turn = y.turn = 0;
    return memcmp(&x, &y, sizeof x) == 0;
}

static int solve(const LevelDef *lv, char *out, int outlen)
{
    static Node nodes[MAX_STATES];
    static const char names[] = "URDLW";
    int head = 0, tail = 0, i, j;

    memset(nodes, 0, sizeof nodes);
    game_load(lv, &nodes[0].s);
    nodes[0].parent = -1;
    tail = 1;

    while (head < tail) {
        int cur = head++;
        for (uint8_t a = 0; a <= ACT_WAIT; a++) {
            GameState n = nodes[cur].s;
            if (!game_step(&n, a)) continue;
            if (n.status == STATUS_CAUGHT) continue;
            for (j = 0; j < tail; j++)
                if (same_state(&nodes[j].s, &n)) break;
            if (j < tail) continue;
            if (tail >= MAX_STATES) return -1;
            nodes[tail].s = n;
            nodes[tail].parent = cur;
            nodes[tail].action = a;
            if (n.status == STATUS_WON) {
                int len = 0, k = tail;
                while (nodes[k].parent >= 0) { len++; k = nodes[k].parent; }
                if (len < outlen) {
                    out[len] = 0;
                    k = tail;
                    for (i = len - 1; i >= 0; i--) {
                        out[i] = names[nodes[k].action];
                        k = nodes[k].parent;
                    }
                }
                return len;
            }
            tail++;
        }
    }
    return 0;  /* unsolvable */
}

static void test_solvable(void)
{
    int l;
    char sol[512];
    for (l = 0; l < LEVEL_COUNT; l++) {
        int len = solve(&levels[l], sol, sizeof sol);
        CHECK(len > 0, "level %d (%s) is not solvable", l + 1, levels[l].name);
        if (len > 0)
            printf("level %d %-12s solved in %3d turns: %s\n",
                   l + 1, levels[l].name, len, sol);
    }
}

int main(void)
{
    test_movement();
    test_vision();
    test_caught_and_undo();
    test_patrol();
    test_level_data();
    if (failures == 0) test_solvable();

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
