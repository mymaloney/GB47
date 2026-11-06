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
#include "../tools/solver.h"

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
    2,
    {
        { DIR_LEFT, 1, { {10, 3} }, DISGUISE_NONE, 0 },  /* sentry looking left at the wall */
        { DIR_UP,   1, { {15, 5} }, DISGUISE_NONE, 0 },  /* sentry looking up */
    },
    { 2, 0 },
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

static void test_throw_targeting(void)
{
    GameState s;
    game_load(&test_level, &s);
    s.px = 3; s.py = 1;
    CHECK(game_can_throw(&s, ITEM_STONE, 9, 1), "6 steps away is in range");
    CHECK(!game_can_throw(&s, ITEM_STONE, 10, 1), "7 steps is out of range");
    CHECK(game_can_throw(&s, ITEM_STONE, 5, 5), "diagonal-ish, 6 steps");
    CHECK(!game_can_throw(&s, ITEM_STONE, 3, 1), "not your own tile");
    CHECK(!game_can_throw(&s, ITEM_STONE, 3, 0), "not into a wall");
    CHECK(!game_can_throw(&s, ITEM_BOTTLE, 4, 1), "no bottles in inventory");
    s.px = 4; s.py = 3;
    CHECK(!game_can_throw(&s, ITEM_STONE, 8, 3), "wall at (6,3) blocks throw");
    CHECK(game_can_throw(&s, ITEM_STONE, 5, 3), "adjacent tile is fine");
    s.status = STATUS_CAUGHT;
    CHECK(game_throw(&s, ITEM_STONE, 5, 3) == 0, "no throwing after caught");
}

static void test_throw_lures_one_guard(void)
{
    GameState s, before;
    int i;
    game_load(&test_level, &s);
    s.px = 3; s.py = 1;
    undo_clear();
    before = s;
    undo_push(&s);

    /* only guard 0 at (10,3) is within 6 of (9,1); guard 1 is 10 away */
    CHECK(game_throw(&s, ITEM_STONE, 9, 1) == 1, "throw spends a turn");
    CHECK(s.turn == 1 && s.items[ITEM_STONE] == 1, "stone used up");
    CHECK(s.guards[0].x == 10 && s.guards[0].y == 3, "noise turn: no step");
    CHECK(s.guards[0].facing == DIR_UP, "turns toward noise (major axis)");
    CHECK(s.guards[0].mode == GUARD_INVESTIGATE, "closest guard investigates");
    CHECK(s.guards[1].mode == GUARD_PATROL && s.guards[1].facing == DIR_UP,
          "guard out of earshot unaffected");

    /* walk path (10,3)->(10,2)->(10,1)->(9,1) */
    for (i = 0; i < 3; i++) game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 9 && s.guards[0].y == 1, "reached the noise");
    CHECK(s.status == STATUS_PLAYING, "player 6 tiles away is unseen");
    for (i = 0; i < 3; i++) game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 10 && s.guards[0].y == 3, "walked back to post");
    CHECK(s.guards[0].mode == GUARD_PATROL, "back on routine");
    CHECK(s.guards[0].facing == DIR_LEFT, "sentry faces its post again");

    while (undo_pop(&s)) {}
    CHECK(memcmp(&s, &before, sizeof s) == 0, "undo returns the stone");
}

static void test_throw_two_hearers(void)
{
    GameState s;
    game_load(&test_level, &s);
    s.px = 9; s.py = 1;
    /* (13,3): guard 0 walks 3, guard 1 walks 4 -> both hear, guard 0 goes */
    CHECK(game_throw(&s, ITEM_STONE, 13, 3) == 1, "throw");
    CHECK(s.guards[0].mode == GUARD_INVESTIGATE, "nearest walker goes");
    CHECK(s.guards[0].facing == DIR_RIGHT, "guard 0 looks right");
    CHECK(s.guards[1].mode == GUARD_PATROL, "other guard stays");
    CHECK(s.guards[1].facing == DIR_LEFT, "but turns to look (tie: horizontal)");
    game_step(&s, ACT_WAIT);
    CHECK(s.guards[1].facing == DIR_UP, "next turn it returns to routine");
    CHECK(s.guards[0].x == 11, "guard 0 heads to the noise");
}

static void test_patroller_resumes_route(void)
{
    GameState s;
    game_load(&levels[0], &s);
    s.items[ITEM_STONE] = 1;
    /* guard at (1,5) walking right toward (18,5); lure it to the door */
    CHECK(game_can_throw(&s, ITEM_STONE, 3, 4), "door in range");
    game_throw(&s, ITEM_STONE, 3, 4);
    CHECK(s.guards[0].mode == GUARD_INVESTIGATE, "patroller investigates");
    while (s.guards[0].mode != GUARD_PATROL && s.turn < 50)
        game_step(&s, ACT_WAIT);
    CHECK(s.status == STATUS_PLAYING, "player in the room stays hidden");
    CHECK(s.guards[0].x == 1 && s.guards[0].y == 5, "returned to where it left");
    CHECK(s.guards[0].facing == DIR_RIGHT && s.guards[0].next_wp == 1,
          "continues its patrol");
    game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 2, "and walks on");
}

/* Open room for takedown tests. Guard 0 faces left (approach from the
 * right). Guard 1 looks down over x 14-16, rows 3-5. */
static const LevelDef td_level = {
    "TD",
    {
        "####################",
        "#..................#",
        "#..................#",
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
        "####################",
    },
    2,
    {
        { DIR_LEFT, 1, { {5, 3} }, DISGUISE_NONE, 0 },
        { DIR_DOWN, 1, { {15, 1} }, DISGUISE_NONE, 0 },
    },
    { 0, 0 },
};

static void test_takedown(void)
{
    GameState s;
    game_load(&td_level, &s);
    s.px = 8; s.py = 3;
    CHECK(game_takedown_target(&s) == 0xFF, "nobody adjacent");
    CHECK(game_step(&s, ACT_TAKEDOWN) == 0, "takedown with nobody near");
    s.px = 6;
    CHECK(game_takedown_target(&s) == 0, "guard 0 adjacent");
    CHECK(game_step(&s, ACT_TAKEDOWN) == 1, "takedown starts");
    CHECK(s.guards[0].mode == GUARD_SUBDUED, "target subdued");
    CHECK(game_step(&s, ACT_LEFT) == 0, "player locked during takedown");
    CHECK(game_step(&s, ACT_DRAG + DIR_LEFT) == 0, "no dragging either");
    CHECK(game_takedown_target(&s) == 0xFF, "no second takedown");
    CHECK(game_step(&s, ACT_WAIT) == 1, "wait 2");
    CHECK(s.guards[0].mode == GUARD_SUBDUED, "still struggling");
    CHECK(game_step(&s, ACT_WAIT) == 1, "wait 3");
    CHECK(s.guards[0].mode == GUARD_BODY && s.takedown_left == 0,
          "after TAKEDOWN_TURNS it is a body");
    CHECK(s.turn == TAKEDOWN_TURNS && s.status == STATUS_PLAYING, "unseen");
    CHECK(game_takedown_target(&s) == 0xFF, "bodies can't be taken down");

    /* walk over the body */
    CHECK(game_step(&s, ACT_LEFT) == 1 && s.px == 5, "step onto body");
    CHECK(game_step(&s, ACT_DRAG + DIR_LEFT) == 0, "no drag while on a body");
    CHECK(game_step(&s, ACT_LEFT) == 1 && s.px == 4, "step off body");

    /* drag: body at (5,3) is behind us when moving left */
    CHECK(game_body_adjacent(&s), "body next to player");
    CHECK(game_step(&s, ACT_DRAG + DIR_LEFT) == 1, "drag");
    CHECK(s.px == 3 && s.guards[0].x == 4 && s.guards[0].y == 3, "body follows");
    /* drag sideways: body moves into the tile we leave */
    CHECK(game_step(&s, ACT_DRAG + DIR_UP) == 1, "drag sideways");
    CHECK(s.px == 3 && s.py == 2 && s.guards[0].x == 3 && s.guards[0].y == 3,
          "body takes our old tile");
    /* push: body straight ahead moves one further */
    CHECK(game_step(&s, ACT_DRAG + DIR_DOWN) == 1, "push");
    CHECK(s.py == 3 && s.guards[0].y == 4, "body pushed down");
    CHECK(game_step(&s, ACT_DRAG + DIR_DOWN) == 1, "push again");
    CHECK(game_step(&s, ACT_DRAG + DIR_DOWN) == 0, "can't push into wall");
    CHECK(game_step(&s, ACT_DOWN) == 1 && s.py == 5, "but can walk onto it");

    /* nothing to drag */
    s.px = 10; s.py = 2;
    CHECK(game_step(&s, ACT_DRAG + DIR_LEFT) == 0, "drag with no body");
}

static void test_body_alarm(void)
{
    GameState s;
    game_load(&td_level, &s);
    /* turn guard 0 into a body, then push it into guard 1's view */
    s.guards[0].mode = GUARD_BODY;
    s.guards[0].x = 12; s.guards[0].y = 4;
    s.px = 11; s.py = 4;
    CHECK(game_step(&s, ACT_DRAG + DIR_RIGHT) == 1, "push");
    CHECK(s.guards[0].x == 13 && s.status == STATUS_PLAYING, "(13,4) unseen");
    CHECK(game_step(&s, ACT_DRAG + DIR_RIGHT) == 1, "push into view");
    CHECK(s.status == STATUS_ALARM, "guard sees body: alarm");

    /* a takedown in plain view of another guard also raises the alarm */
    game_load(&td_level, &s);
    s.guards[0].x = 14; s.guards[0].y = 5; s.guards[0].facing = DIR_RIGHT;
    s.px = 13; s.py = 5;
    CHECK(game_step(&s, ACT_TAKEDOWN) == 1, "takedown");
    CHECK(s.status == STATUS_ALARM, "guard 1 saw the struggle");
}

/* Disguise tests. Guard 0 at (2,3) faces right: hostile to own clothes,
 * blind to GUARD. Guard 1 patrols row 6: enforcer to own clothes, blind to
 * GUARD. Both wear GUARD uniforms. */
static const LevelDef dz_level = {
    "DZ",
    {
        "####################",
        "#..................#",
        "#..................#",
        "#..................#",
        "#..................#",
        "#..................#",
        "#..................#",
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
    2,
    {
        { DIR_RIGHT, 1, { {2, 3} },           DISGUISE_GUARD, "HBE" },
        { DIR_RIGHT, 2, { {2, 6}, {17, 6} },  DISGUISE_GUARD, "EBE" },
    },
    { 0, 0 },
};

static void test_disguise_cones(void)
{
    static uint8_t vis[MAP_H][MAP_W];
    GameState s;

    game_load(&levels[0], &s);
    CHECK(game_attitude(&s, 0) == ATT_ENFORCER, "no attitude string = enforcer");

    /* hostile: 7 deep, up to 7 wide */
    game_load(&dz_level, &s);
    CHECK(game_attitude(&s, 0) == ATT_HOSTILE, "hostile to own clothes");
    s.guards[1].x = 17;   /* move guard 1's cone out of the way */
    game_compute_vision(&s, vis);
    CHECK(vis[3][9] == VIS_SEEN, "hostile cone reaches 7");
    CHECK(vis[3][10] == VIS_NONE, "but not 8");
    CHECK(vis[6][9] == VIS_SEEN && vis[1][9] == VIS_SEEN, "7 tiles wide");
    CHECK(vis[4][4] == VIS_SEEN && vis[5][4] == VIS_NONE, "3 wide at 2");
    CHECK(vis[5][5] == VIS_SEEN && vis[6][5] == VIS_NONE, "5 wide at 3");
    s.px = 9; s.py = 3;
    game_step(&s, ACT_WAIT);
    CHECK(s.status == STATUS_CAUGHT, "spotted at distance 7");

    /* blind: no cone for the player, normal cone still watching */
    game_load(&dz_level, &s);
    s.disguise = DISGUISE_GUARD;
    CHECK(game_attitude(&s, 0) == ATT_BLIND, "blind to GUARD");
    game_compute_vision(&s, vis);
    CHECK(vis[3][5] == VIS_WATCHED, "blind guard's cone shown as watched");
    CHECK(vis[3][7] == VIS_NONE, "watch cone is the normal size");
    s.px = 4; s.py = 3;
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_PLAYING,
          "standing right in front of a blind guard is fine");
}

static void test_disguise_suspicious(void)
{
    GameState s;

    /* throwing in a blind guard's normal cone gets you caught */
    game_load(&dz_level, &s);
    s.disguise = DISGUISE_GUARD;
    s.items[ITEM_STONE] = 2;
    s.px = 5; s.py = 3;
    CHECK(game_throw(&s, ITEM_STONE, 5, 1) && s.status == STATUS_CAUGHT,
          "throwing is suspicious");
    game_load(&dz_level, &s);
    s.disguise = DISGUISE_GUARD;
    s.items[ITEM_STONE] = 2;
    s.px = 7; s.py = 3;
    CHECK(game_throw(&s, ITEM_STONE, 7, 1) && s.status == STATUS_PLAYING,
          "outside the normal cone it's fine");

    /* bodies are still spotted */
    game_load(&dz_level, &s);
    s.disguise = DISGUISE_GUARD;
    s.px = 12; s.py = 1;
    s.guards[1].mode = GUARD_BODY;
    s.guards[1].x = 4; s.guards[1].y = 3;
    game_step(&s, ACT_WAIT);
    CHECK(s.status == STATUS_ALARM, "blind guard still sees bodies");

    /* a blind guard walking into you just waits */
    game_load(&dz_level, &s);
    s.disguise = DISGUISE_GUARD;
    s.px = 4; s.py = 6;
    game_step(&s, ACT_WAIT);
    CHECK(s.guards[1].x == 3, "patrol moves up to you");
    game_step(&s, ACT_WAIT);
    CHECK(s.guards[1].x == 3 && s.status == STATUS_PLAYING, "and waits");
    game_step(&s, ACT_UP);
    game_step(&s, ACT_WAIT);
    CHECK(s.guards[1].x == 5, "then walks on once you step aside");
}

static void test_take_uniform(void)
{
    GameState s;
    game_load(&dz_level, &s);
    s.px = 12; s.py = 1;
    CHECK(game_uniform_here(&s) == 0xFF, "no body here");
    CHECK(game_step(&s, ACT_UNIFORM) == 0, "nothing to take");
    s.guards[1].mode = GUARD_BODY;
    s.guards[1].x = 12; s.guards[1].y = 1;
    CHECK(game_uniform_here(&s) == 1, "standing on a uniformed body");
    CHECK(game_step(&s, ACT_UNIFORM) == 1, "take it (spends a turn)");
    CHECK(s.disguise == DISGUISE_GUARD && s.guards[1].stripped, "wearing it");
    CHECK(s.status == STATUS_PLAYING, "nobody saw");
    CHECK(game_uniform_here(&s) == 0xFF, "a body has one uniform");

    /* bodies from levels without uniforms give nothing */
    game_load(&td_level, &s);
    s.guards[0].mode = GUARD_BODY;
    s.px = s.guards[0].x; s.py = s.guards[0].y;
    CHECK(game_uniform_here(&s) == 0xFF, "no uniform defined");
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
        CHECK(strlen(lv->name) <= 12, "level %d name too long", l + 1);
        game_load(lv, &s);
        for (i = 0; i < lv->guard_count; i++) {
            const GuardDef *d = &lv->guards[i];
            CHECK(d->attitude == 0 || strlen(d->attitude) == DISGUISE_TYPES,
                  "level %d guard %d attitude must have %d letters", l + 1, i,
                  DISGUISE_TYPES);
            CHECK(d->attitude == 0 || strspn(d->attitude, "HEB") == DISGUISE_TYPES,
                  "level %d guard %d attitude letters are H, E or B", l + 1, i);
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

/* ---- Solvability (tools/solver.c) ---- */

static int solve(const LevelDef *lv, int use, char *out, int outlen)
{
    static SolveStep steps[1024];
    int states, len = solver_run(lv, use, steps, 1024, &states);
    if (getenv("SOLVER_STATS"))
        fprintf(stderr, "  [%s: %d states]\n", lv->name, states);
    out[0] = 0;
    if (len > 0) solver_format(steps, len < 1024 ? len : 1024, out, outlen);
    return len;
}

static void test_solvable(void)
{
    int l;
    char sol[1024];
    for (l = 0; l < LEVEL_COUNT; l++) {
        int len = solve(&levels[l], SOLVE_USE_ALL, sol, sizeof sol);
        CHECK(len > 0, "level %d (%s) is not solvable (%d)", l + 1,
              levels[l].name, len);
        if (len > 0)
            printf("level %d %-12s solved in %3d turns: %s\n",
                   l + 1, levels[l].name, len, sol);
    }
}

/* Each level must actually need the mechanics it teaches: with that
 * mechanic switched off, the solver must fail. The level editor can show
 * the same check for any level. */
static const struct {
    int  level;        /* 1-based */
    int  without;      /* SOLVE_USE_* bits to switch off */
    const char *what;
} required[] = {
    { 4, SOLVE_USE_ITEMS,    "items" },
    { 5, SOLVE_USE_TAKEDOWN, "a takedown" },
    { 5, SOLVE_USE_DRAG,     "dragging the body" },
    { 6, SOLVE_USE_UNIFORM,  "a disguise" },
};

static void test_mechanics_needed(void)
{
    unsigned r;
    char sol[1024];
    for (r = 0; r < sizeof required / sizeof required[0]; r++) {
        const LevelDef *lv = &levels[required[r].level - 1];
        CHECK(solve(lv, SOLVE_USE_ALL & ~required[r].without, sol, sizeof sol) == 0,
              "level %d (%s) can be beaten without %s: %s",
              required[r].level, lv->name, required[r].what, sol);
    }
}

int main(void)
{
    test_movement();
    test_vision();
    test_caught_and_undo();
    test_patrol();
    test_throw_targeting();
    test_throw_lures_one_guard();
    test_throw_two_hearers();
    test_patroller_resumes_route();
    test_takedown();
    test_body_alarm();
    test_disguise_cones();
    test_disguise_suspicious();
    test_take_uniform();
    test_level_data();
    if (failures == 0) {
        test_solvable();
        test_mechanics_needed();
    }

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
