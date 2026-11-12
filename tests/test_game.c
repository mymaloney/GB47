/*
 * test_game.c - Host-side tests for the game rules (compiled with gcc).
 *
 *   make test
 *
 * Besides unit checks, this runs the A* solver over every level to
 * prove it can be completed, and prints the shortest solution. Run it every
 * time you edit a level.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/game.h"
#include "../src/levels.h"
#include "../tools/solver.h"
#include "../src/world.h"
#include "../src/trans.h"

static int failures = 0;

/* Access layers for 20x16 test maps (run, byte pairs; two bits per
 * disguise: own clothes, GUARD, OFFICER). */
#define ACC(suit, guard, officer) ((suit) | (guard) << 2 | (officer) << 4)
static const uint8_t all_legal[] = { 255, 0, 65, 0 };

/* For test maps without switch cells. */
#define NO_SCRIPTING 0, { { 0, 0, 0, 0, 0, 0 } }

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
    20, 16,
        "####################"
        "#P.................#"
        "#..................#"
        "#.....#............#"
        "#..................#"
        "#..................#"
        "#.................E#"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################",
    2,
    {
        { DIR_LEFT, 1, { {10, 3} }, DISGUISE_NONE, 0 },  /* sentry looking left at the wall */
        { DIR_UP,   1, { {15, 5} }, DISGUISE_NONE, 0 },  /* sentry looking up */
    },
    { 2, 0 },
    0, { 0 }, { 0 },   /* single map: no links */
    TRANS_CUT, NO_SCRIPTING
};

static void test_movement(void)
{
    GameState s;
    game_load(&test_level, &s);
    for (int i = 0; i < ITEM_TYPES; i++)
        CHECK(item_defs[i].range <= SOLVE_MAX_RANGE,
              "%s out-ranges the solver: raise SOLVE_MAX_RANGE", item_defs[i].name);
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
    World w, before;
    GameState *s;
    int i;
    world_load(&test_level, 0, &w);
    s = world_state(&w);
    undo_clear();
    /* walk right along row 2; (6,2) is the far corner of the cone */
    world_step(&w, ACT_DOWN);
    for (i = 0; i < 4; i++) { undo_push(&w); world_step(&w, ACT_RIGHT); }
    CHECK(s->px == 5 && s->status == STATUS_PLAYING, "not yet seen at (5,2)");
    before = w;
    undo_push(&w);
    world_step(&w, ACT_RIGHT);
    CHECK(s->status == STATUS_CAUGHT, "caught entering cone");
    CHECK(world_step(&w, ACT_RIGHT) == 0, "no moves after caught");
    CHECK(undo_pop(&w) == 1, "undo works");
    CHECK(memcmp(&w, &before, WORLD_BYTES(&w)) == 0, "undo restores state");

    /* a marked turn that didn't happen costs no history, even when full:
     * exactly UNDO_DEPTH turns can be taken back */
    world_load(&test_level, 0, &w);
    undo_clear();
    for (i = 0; i < UNDO_DEPTH + 3; i++) {
        undo_mark(&w);
        CHECK(world_step(&w, ACT_UP) == 0, "wall");     /* not committed */
        undo_mark(&w);
        if (world_step(&w, ACT_WAIT)) undo_commit();
    }
    CHECK(s->turn == UNDO_DEPTH + 3, "waited");
    for (i = 0; i < UNDO_DEPTH; i++) CHECK(undo_pop(&w) == 1, "undo %d", i);
    CHECK(undo_pop(&w) == 0 && s->turn == 3, "history holds UNDO_DEPTH turns");
}

/* The original level 1 layout, fixed here so these tests don't change
 * when the real level 1 is edited: a start room, a doorway at (3,4), a
 * hallway patrolled from (1,5) to (18,5), and the exit room. */
static const LevelDef patrol_level = {
    "PATROL", 20, 16,
        "####################"
        "#P.....#############"
        "#......#############"
        "#......#############"
        "###.################"
        "#..................#"
        "#..................#"
        "############.#######"
        "##########.....#####"
        "##########...E.#####"
        "##########.....#####"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################",
    1,
    { { DIR_RIGHT, 2, { {1, 5}, {18, 5} }, DISGUISE_NONE, 0 } },
    { 0, 0 },
    0, { 0 }, { 0 },   /* single map: no links */
    TRANS_CUT, NO_SCRIPTING
};

static void test_patrol(void)
{
    GameState s;
    game_load(&patrol_level, &s);
    /* the guard walks right from (1,5) */
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
    World w, before;
    GameState s;
    int i;
    world_load(&test_level, 0, &w);
    w.part[0].px = 3; w.part[0].py = 1;
    undo_clear();
    before = w;
    undo_push(&w);
    s = w.part[0];

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

    w.part[0] = s;
    while (undo_pop(&w)) {}
    CHECK(memcmp(&w, &before, WORLD_BYTES(&w)) == 0, "undo returns the stone");
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
    game_load(&patrol_level, &s);
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
    20, 16,
        "####################"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#.................E#"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################",
    2,
    {
        { DIR_LEFT, 1, { {5, 3} }, DISGUISE_NONE, 0 },
        { DIR_DOWN, 1, { {15, 1} }, DISGUISE_NONE, 0 },
    },
    { 0, 0 },
    0, { 0 }, { 0 },   /* single map: no links */
    TRANS_CUT, NO_SCRIPTING
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

/* Disguise tests. The whole map is hostile to your own clothes, legal in a
 * GUARD uniform and trespassing in an OFFICER one. Guard 0 at (2,3) faces
 * right; guard 1 patrols row 6. Both wear GUARD uniforms. */
static const uint8_t dz_access[] = {
    255, ACC(ACCESS_HOSTILE, ACCESS_LEGAL, ACCESS_TRESPASS),
    65,  ACC(ACCESS_HOSTILE, ACCESS_LEGAL, ACCESS_TRESPASS),
};
static const LevelDef dz_level = {
    "DZ",
    20, 16,
        "####################"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################"
        "####################",
    2,
    {
        { DIR_RIGHT, 1, { {2, 3} },           DISGUISE_GUARD, 0 },
        { DIR_RIGHT, 2, { {2, 6}, {17, 6} },  DISGUISE_GUARD, 0 },
    },
    { 0, 0 },
    0, { 0 }, { 0 },   /* single map: no links */
    TRANS_CUT, NO_SCRIPTING, dz_access
};

static void test_disguise_cones(void)
{
    static uint8_t vis[VIEW_H][VIEW_W];
    GameState s;

    game_load(&levels[0], &s);
    CHECK(game_access(&s, s.px, s.py) == ACCESS_TRESPASS && game_regard(&s, 0) == ACCESS_TRESPASS,
          "no access layer: your own clothes trespass");
    s.disguise = DISGUISE_GUARD;
    CHECK(game_access(&s, s.px, s.py) == ACCESS_LEGAL, "and uniforms are legal");

    /* hostile: 7 deep, up to 7 wide */
    game_load(&dz_level, &s);
    CHECK(game_access(&s, 9, 3) == ACCESS_HOSTILE, "hostile to own clothes");
    s.guards[1].x = 17;   /* move guard 1's cone out of the way */
    game_compute_vision(&s, 0, 0, VIEW_W, VIEW_H, &vis[0][0]);
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
    CHECK(game_access(&s, 4, 3) == ACCESS_LEGAL && game_regard(&s, 0) == ACCESS_LEGAL,
          "a GUARD uniform is legal here");
    game_compute_vision(&s, 0, 0, VIEW_W, VIEW_H, &vis[0][0]);
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

/* Two screens wide, more than one tall. A wall splits it at x=20 with a
 * single gap at the bottom (y=17). Guard 0 stands right of the wall. */
static const LevelDef big_level = {
    "BIG",
    40, 20,
        "########################################"
        "#...................#..................#"
        "#...................#..................#"
        "#...................#..................#"
        "#............P......#..................#"
        "#...................#..................#"
        "#...................#..................#"
        "#...................#..................#"
        "#...................#..................#"
        "#...................#..................#"
        "#...................#................E.#"
        "#...................#..................#"
        "#...................#..................#"
        "#...................#..................#"
        "#...................#..................#"
        "#...................#..................#"
        "#...................#..................#"
        "#......................................#"
        "#...................#..................#"
        "########################################",
    1,
    {
        { DIR_RIGHT, 1, { {24, 4} }, DISGUISE_NONE, 0 },
    },
    { 1, 0 },
    0, { 0 }, { 0 },   /* single map: no links */
    TRANS_CUT, NO_SCRIPTING
};

static void test_big_map(void)
{
    static uint8_t full[20][40], part[VIEW_H][VIEW_W];
    GameState s;
    int x, y, ok = 1, turns;

    game_load(&big_level, &s);
    CHECK(game_width() == 40 && game_height() == 20, "size");
    CHECK(s.px == 13 && s.py == 4, "player start found in a flat map");
    CHECK(game_cell(25, 5) == CELL_FLOOR && game_cell(20, 5) == CELL_WALL, "cells");
    CHECK(game_cell(37, 10) == CELL_EXIT, "exit beyond the first screen");
    CHECK(game_cell(40, 5) == CELL_WALL && game_cell(5, 20) == CELL_WALL,
          "outside the map is wall");

    /* vision for a screen-sized block matches the same tiles of the whole */
    game_compute_vision(&s, 0, 0, 40, 20, &full[0][0]);
    game_compute_vision(&s, 20, 4, VIEW_W, VIEW_H, &part[0][0]);
    for (y = 0; y < VIEW_H; y++)
        for (x = 0; x < VIEW_W; x++)
            if (part[y][x] != full[y + 4][x + 20]) ok = 0;
    CHECK(ok, "partial vision block matches full map");
    CHECK(full[4][28] == VIS_SEEN && full[4][29] == VIS_NONE, "cone in screen 2");

    /* noise across the wall: the guard hears it (hearing ignores walls)
     * and walks the long way round through the gap, then back */
    CHECK(game_throw(&s, ITEM_STONE, 18, 4), "throw");
    CHECK(s.guards[0].mode == GUARD_INVESTIGATE, "guard heard it");
    for (turns = 0; turns < 60 && !(s.guards[0].x == 18 && s.guards[0].y == 4); turns++)
        game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 18 && s.guards[0].y == 4, "reached the noise");
    CHECK(turns == 32, "via the gap: 13 down, 6 across, 13 up (took %d)", turns);
    CHECK(s.status == STATUS_PLAYING, "player unseen");
    for (turns = 0; turns < 60 && s.guards[0].mode != GUARD_PATROL; turns++)
        game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 24 && s.guards[0].y == 4, "and walked back to its post");
}

/* ---- Tile types. One open room whose map is a writable buffer, so each
 * test can build a column of some tile at x=7. Guard 0 stands at (10,8)
 * facing left, toward the column. ---- */
static char tile_map[20 * 16 + 1];
static const LevelDef tile_level = {
    "TILES", 20, 16, tile_map,
    1,
    { { DIR_LEFT, 1, { {10, 8} }, DISGUISE_NONE, 0 } },
    { 2, 0 },
    0, { 0 }, { 0 },   /* single map: no links */
    TRANS_CUT, NO_SCRIPTING
};

static void tiles_reset(char column, char gap)
{
    int x, y;
    for (y = 0; y < 16; y++)
        for (x = 0; x < 20; x++)
            tile_map[y * 20 + x] = (x == 0 || y == 0 || x == 19 || y == 15) ? '#' : '.';
    if (column)
        for (y = 1; y < 15; y++) tile_map[y * 20 + 7] = column;
    if (gap) tile_map[8 * 20 + 7] = gap;
}

static void tiles_load(GameState *s, int px, int py)
{
    game_load(&tile_level, s);
    s->px = (uint8_t)px;
    s->py = (uint8_t)py;
}

/* Level transitions: every screen tile changes exactly once, at a step
 * inside the animation, and the animation has no dead steps at either end. */
static void test_transitions(void)
{
    static const uint8_t centres[][2] = { {10, 8}, {0, 0}, {19, 15}, {1, 7}, {18, 1} };
    unsigned c;
    int kind, in, x, y;

    CHECK(trans_steps(TRANS_CUT, 10, 8) == 0 && trans_steps(TRANS_FADE, 10, 8) == 0 &&
          trans_steps(TRANS_COUNT, 10, 8) == 0, "cut and fade aren't tile patterns");
    for (kind = TRANS_FADE + 1; kind < TRANS_COUNT; kind++)
        for (c = 0; c < sizeof centres / sizeof centres[0]; c++) {
            uint8_t cx = centres[c][0], cy = centres[c][1];
            uint8_t n = trans_steps((uint8_t)kind, cx, cy);
            CHECK(n >= 4 && n <= 40, "transition %d has %d steps", kind, n);
            for (in = 0; in < 2; in++) {
                int first = 0, last = 0;
                for (y = 0; y < TRANS_H; y++)
                    for (x = 0; x < TRANS_W; x++) {
                        uint8_t p = trans_phase((uint8_t)kind, (uint8_t)in, n, (uint8_t)x,
                                                (uint8_t)y, cx, cy);
                        CHECK(p < n, "transition %d: tile (%d,%d) at step %d of %d",
                              kind, x, y, p, n);
                        if (p == 0) first = 1;
                        if (p == n - 1) last = 1;
                    }
                CHECK(first && last, "transition %d (in=%d, centre %d,%d) has dead steps",
                      kind, in, cx, cy);
            }
        }
    /* the iris closes on the player: their tile is the last to go, the
     * first to come back */
    {
        uint8_t n = trans_steps(TRANS_IRIS, 3, 12);
        CHECK(trans_phase(TRANS_IRIS, 0, n, 3, 12, 3, 12) == n - 1 &&
              trans_phase(TRANS_IRIS, 1, n, 3, 12, 3, 12) == 0, "iris centred on the player");
    }
    /* every level names a transition that exists */
    for (kind = 0; kind < LEVEL_COUNT; kind++)
        CHECK(levels[kind].transition < TRANS_COUNT, "map %d: unknown transition", kind);
}

/* Characters you can talk to, scripts and switch cells. A dog at (5,3)
 * in an open room; petting it sets a flag that turns (10,3) from floor into
 * the exit and (12,3) from floor into wall. A sign at (5,8) says two
 * things; a statue at (15,3) has nothing to say. */
static const uint8_t pet_script[] = {
    OP_IF, 1, 4,   OP_SAY, 7, OP_JUMP, 4,      /* petted before: "wags its tail" */
                   OP_SAY, 3, OP_SET, 1,       /* else: "you pet the dog", set flag 0 */
    OP_END,
};
static const uint8_t two_lines[] = { OP_SAY, 1, OP_SAY, 2, OP_CLEAR, 1, OP_END };
static char npc_map[20 * 16 + 1];
static const LevelDef npc_level = {
    "NPC", 20, 16, npc_map,
    4,
    { { DIR_UP, 1, { {17, 13} }, DISGUISE_NONE, 0 },      /* far away, looking at a wall */
      { DIR_LEFT, 1, { {5, 3} }, DISGUISE_NONE, 0, CHAR_NO_TAKEDOWN | CHAR_NO_SIGHT, 0, "PET", pet_script },
      { DIR_DOWN, 1, { {5, 8} }, DISGUISE_NONE, 0, CHAR_NO_TAKEDOWN | CHAR_NO_SIGHT, 0, "READ", two_lines },
      { DIR_DOWN, 1, { {15, 3} }, DISGUISE_NONE, 0, CHAR_NO_TAKEDOWN | CHAR_NO_SIGHT, 0, 0, 0 } },
    { 0, 0 },
    0, { 0 }, { 0 }, TRANS_CUT,
    2,
    { { 10, 3, 1, CELL_FLOOR, CELL_EXIT, 0xFF },
      { 12, 3, 1, CELL_FLOOR, CELL_WALL, 0xFF } },
};

static void npc_reset(void)
{
    int x, y;
    for (y = 0; y < 16; y++)
        for (x = 0; x < 20; x++)
            npc_map[y * 20 + x] = (x == 0 || y == 0 || x == 19 || y == 15) ? '#' : '.';
    npc_map[3 * 20 + 1] = 'P';
    npc_map[3 * 20 + 10] = '0';
    npc_map[3 * 20 + 12] = '1';
}

static void test_npcs(void)
{
    World w, before;
    GameState *s;
    SolveStep steps[64];
    char text[256];
    uint8_t vis[20 * 16];
    int n;

    npc_reset();
    world_load(&npc_level, 0, &w);
    s = world_state(&w);
    undo_clear();

    CHECK(game_talk_target(s) == 0xFF && world_step(&w, ACT_TALK) == 0 && s->turn == 0,
          "nobody to talk to from the start");
    world_step(&w, ACT_RIGHT); world_step(&w, ACT_RIGHT); world_step(&w, ACT_RIGHT);
    CHECK(s->px == 4 && s->status == STATUS_PLAYING && game_talk_target(s) == 1,
          "next to the dog, right in front of it, and not caught");
    game_compute_vision(s, 0, 0, 20, 16, vis);
    CHECK(vis[3 * 20 + 4] == VIS_NONE && vis[3 * 20 + 3] == VIS_NONE,
          "a character that ignores you has no cone");
    CHECK(world_step(&w, ACT_RIGHT) == 0 && s->px == 4, "the dog is in the way");
    CHECK(!game_blocks(5, 3, BLOCK_SIGHT | BLOCK_THROW), "but not in the way of sight or throws");
    CHECK(game_takedown_target(s) == 0xFF && world_step(&w, ACT_TAKEDOWN) == 0,
          "you can't take down someone you can talk to");
    CHECK(game_cell(10, 3) == CELL_FLOOR && game_cell(12, 3) == CELL_FLOOR, "switch cells off");

    before = w;
    undo_push(&w);
    CHECK(world_step(&w, ACT_TALK) == 1 && s->turn == 4, "petting costs a turn");
    CHECK(game_said_count() == 1 && game_said(0) == 3, "it says the first-time line");
    CHECK(s->flags == 1 && s->status == STATUS_PLAYING, "flag set");
    CHECK(game_cell(10, 3) == CELL_EXIT && game_cell(12, 3) == CELL_WALL,
          "switch cells follow the flag");
    CHECK(world_step(&w, ACT_WAIT) == 1 && game_said_count() == 0, "messages last one turn");
    CHECK(world_step(&w, ACT_TALK) == 1 && game_said_count() == 1 && game_said(0) == 7,
          "the second time it says the other line");

    /* undo takes the flag, and the exit, back */
    w = before; world_activate(&w);       /* (as if two more undos) */
    world_step(&w, ACT_TALK);
    CHECK(undo_pop(&w) == 1 && s->flags == 0 && game_cell(10, 3) == CELL_FLOOR,
          "undo hides the exit again");
    CHECK(memcmp(&w, &before, WORLD_BYTES(&w)) == 0, "undo restores the state exactly");

    /* two messages in one turn; talking from any side */
    s->px = 5; s->py = 9;
    CHECK(game_talk_target(s) == 2, "next to the sign (from below)");
    CHECK(world_step(&w, ACT_TALK) == 1 && game_said_count() == 2 &&
          game_said(0) == 1 && game_said(1) == 2, "two messages in one turn");
    s->px = 14; s->py = 3;
    CHECK(game_talk_target(s) == 0xFF && world_step(&w, ACT_TALK) == 0,
          "a character without a script can't be talked to");
    CHECK(game_takedown_target(s) == 0xFF, "and this one can't be taken down either");

    /* a body right in front of a character that ignores you: no alarm */
    s->guards[0].mode = GUARD_BODY;
    s->guards[0].x = 4; s->guards[0].y = 3;
    s->px = 7; s->py = 7;
    CHECK(world_step(&w, ACT_WAIT) == 1 && s->status == STATUS_PLAYING,
          "a character that ignores you ignores bodies too");
    s->guards[0].mode = GUARD_PATROL;
    s->guards[0].x = 17; s->guards[0].y = 13;

    /* guards path round switch cells as they change */
    s->px = 4; s->py = 3;
    world_step(&w, ACT_TALK);
    CHECK(game_blocks(12, 3, BLOCK_GUARD), "the new wall blocks guards");

    /* the level can't be finished without the dog, and the solver finds it */
    npc_reset();
    CHECK(solver_run(&npc_level, 0, SOLVE_USE_ALL & ~SOLVE_USE_TALK, NULL, 0, NULL) == 0,
          "no exit without talking");
    n = solver_run(&npc_level, 0, SOLVE_USE_ALL, steps, 64, NULL);
    solver_format(steps, n, text, sizeof text);
    /* 3 steps to the dog, pet it, then 8 round it to the new exit */
    CHECK(n == 12 && strstr(text, "TALK"), "solved with a talk in %d: %s", n, text);
}

/* A character that talks can also walk a patrol (talk to it wherever it
 * is), and next to someone you can talk to, B talks: no takedown of the
 * guard beside them that turn. */
static void test_walking_talker(void)
{
    static const uint8_t hello[] = { OP_SAY, 0, OP_END };
    static const LevelDef lv = {
        "WALKER", 20, 16, tile_map,
        2,
        { { DIR_RIGHT, 2, { {3, 7}, {12, 7} }, DISGUISE_NONE, 0, CHAR_NO_TAKEDOWN | CHAR_NO_SIGHT, 0,
            "TALK", hello },
          { DIR_DOWN, 1, { {1, 9} }, DISGUISE_NONE, 0 } },    /* (you're legal here: ignored) */
        { 0, 0 },
        0, { 0 }, { 0 }, TRANS_CUT, NO_SCRIPTING, all_legal
    };
    GameState s;
    tiles_reset(0, 0);                           /* an open room */
    game_load(&lv, &s);
    s.px = 5; s.py = 8;                          /* below the walker's path */
    CHECK(game_talk_target(&s) == 0xFF, "the walker is two tiles away");
    game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 4 && game_talk_target(&s) == 0xFF, "it walks");
    game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 5 && game_talk_target(&s) == 0, "now it's above you");
    CHECK(game_step(&s, ACT_TALK) == 1 && game_said_count() == 1, "talk to it");
    CHECK(s.guards[0].x == 6, "and it walks on (talking costs a turn)");
    /* the blind guard can be taken down, except while the walker is beside you */
    s.px = 1; s.py = 8;
    s.guards[0].x = 2; s.guards[0].y = 8;
    CHECK(game_talk_target(&s) == 0 && game_takedown_target(&s) == 0xFF,
          "B talks while someone talkable is next to you");
    s.guards[0].x = 12; s.guards[0].y = 7;
    CHECK(game_takedown_target(&s) == 1, "otherwise the blind guard can be taken down");
}

/* Access zones. An open room; for your own clothes the left part (x < 8)
 * is legal, the middle trespassing and the right part (x >= 14) hostile.
 * A GUARD uniform is legal everywhere, an OFFICER one trespasses
 * everywhere. Guard 0 at (2,8) faces right and just enforces the rules;
 * guard 1 at (2,12) faces right and is an enforcer for your own clothes
 * and for GUARD uniforms. */
static uint8_t zone_access[2 * 20 * 16];
static const LevelDef zone_level = {
    "ZONES", 20, 16, tile_map,
    2,
    { { DIR_RIGHT, 1, { {2, 8} },  DISGUISE_GUARD, 0 },
      { DIR_RIGHT, 1, { {2, 12} }, DISGUISE_GUARD,
        (1 << DISGUISE_NONE) | (1 << DISGUISE_GUARD) } },
    { 2, 0 },
    0, { 0 }, { 0 }, TRANS_CUT, NO_SCRIPTING, zone_access
};

static void zone_load(GameState *s, int px, int py)
{
    int x, y, n = 0;
    tiles_reset(0, 0);
    for (y = 0; y < 16; y++)            /* one run per cell: simple, and legal */
        for (x = 0; x < 20; x++) {
            zone_access[n++] = 1;
            zone_access[n++] = ACC(x < 8 ? ACCESS_LEGAL : x < 14 ? ACCESS_TRESPASS : ACCESS_HOSTILE,
                                   ACCESS_LEGAL, ACCESS_TRESPASS);
        }
    game_load(&zone_level, s);
    s->px = (uint8_t)px;
    s->py = (uint8_t)py;
}

static void test_access(void)
{
    static uint8_t vis[VIEW_H][VIEW_W];
    GameState s;
    int i;

    /* the rules for a cell depend on what you wear */
    zone_load(&s, 4, 8);
    CHECK(game_access(&s, 4, 8) == ACCESS_LEGAL && game_access(&s, 8, 8) == ACCESS_TRESPASS &&
          game_access(&s, 14, 8) == ACCESS_HOSTILE, "three zones for your own clothes");
    CHECK(game_access(&s, 40, 8) == ACCESS_TRESPASS, "off the map counts as trespassing");
    s.disguise = DISGUISE_OFFICER;
    CHECK(game_access(&s, 4, 8) == ACCESS_TRESPASS, "an OFFICER uniform trespasses even there");

    /* legal: right in front of a guard is fine; it only watches */
    zone_load(&s, 4, 8);
    CHECK(game_regard(&s, 0) == ACCESS_LEGAL, "guard 0 lets you be");
    game_compute_vision(&s, 0, 0, VIEW_W, VIEW_H, &vis[0][0]);
    CHECK(vis[8][3] == VIS_WATCHED && vis[8][6] == VIS_WATCHED && vis[8][7] == VIS_NONE,
          "its cone is shown as watching, normal size");
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_PLAYING, "standing in a legal area");
    CHECK(game_throw(&s, ITEM_STONE, 4, 5) && s.status == STATUS_CAUGHT,
          "but suspicious acts are still noticed");

    /* the same guard catches you the moment you cross the line */
    zone_load(&s, 7, 8);
    s.guards[0].x = 5;                       /* its cone reaches x = 9 */
    game_compute_vision(&s, 0, 0, VIEW_W, VIEW_H, &vis[0][0]);
    CHECK(vis[8][7] == VIS_WATCHED && vis[8][8] == VIS_SEEN && vis[8][9] == VIS_SEEN,
          "one cone, watching the legal part and seeing the rest");
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_PLAYING, "legal side of the line");
    CHECK(game_step(&s, ACT_RIGHT) && s.status == STATUS_CAUGHT, "trespassing in its cone");
    zone_load(&s, 7, 8);
    s.guards[0].x = 5;
    s.disguise = DISGUISE_GUARD;
    CHECK(game_step(&s, ACT_RIGHT) && s.status == STATUS_PLAYING, "not in a GUARD uniform");

    /* hostile: the big cone, but only for cells that are hostile */
    zone_load(&s, 15, 8);
    s.guards[0].x = 9;                       /* 6 away from the player */
    game_compute_vision(&s, 0, 0, VIEW_W, VIEW_H, &vis[0][0]);
    CHECK(vis[8][13] == VIS_SEEN && vis[8][14] == VIS_SEEN && vis[8][16] == VIS_SEEN &&
          vis[8][17] == VIS_NONE, "normal cone to x=13, big cone over the hostile part");
    CHECK(vis[6][13] == VIS_NONE && vis[6][14] == VIS_SEEN,
          "the big cone's width only counts on hostile cells");
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_CAUGHT, "seen from 6 away in a hostile area");
    zone_load(&s, 13, 8);
    s.guards[0].x = 8;                       /* 5 away, on a trespassing cell */
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_PLAYING,
          "5 away is out of the normal cone when only trespassing");

    /* an enforcer isn't fooled where the outfit is legal */
    zone_load(&s, 4, 12);
    CHECK(game_regard(&s, 1) == ACCESS_TRESPASS && game_regard(&s, 0) == ACCESS_LEGAL,
          "guard 1 is an enforcer for your own clothes");
    game_compute_vision(&s, 0, 0, VIEW_W, VIEW_H, &vis[0][0]);
    CHECK(vis[12][4] == VIS_SEEN, "so its cone is a seeing one");
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_CAUGHT, "and it catches you");
    zone_load(&s, 4, 12);
    s.disguise = DISGUISE_GUARD;
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_CAUGHT, "and sees through a GUARD uniform");
    zone_load(&s, 4, 8);
    s.disguise = DISGUISE_GUARD;
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_PLAYING, "which fools guard 0");

    /* a guard walking into you waits if you're allowed there */
    zone_load(&s, 3, 8);
    for (i = 0; i < 3; i++) game_step(&s, ACT_WAIT);
    CHECK(s.status == STATUS_PLAYING, "nose to nose with a guard, legally");

    /* bodies are spotted wherever they lie, with the normal cone */
    zone_load(&s, 1, 1);
    s.guards[1].mode = GUARD_BODY;
    s.guards[1].x = 8; s.guards[1].y = 8;    /* 6 away from guard 0 */
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_PLAYING, "a body out of the cone");
    s.guards[1].x = 6;
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_ALARM, "a body in a legal area is still a body");
}

/* A patrol leg with something in the way (the editor refuses these, but
 * the rules shouldn't leave the guard stuck): it walks round. */
static void test_patrol_detour(void)
{
    static const LevelDef blocked = {
        "DETOUR", 20, 16, tile_map,
        1,
        { { DIR_RIGHT, 2, { {3, 3}, {12, 3} }, DISGUISE_NONE, 0 } },
        { 0, 0 },
        0, { 0 }, { 0 }, TRANS_CUT, NO_SCRIPTING
    };
    GameState s;
    int i;

    tiles_reset(0, 0);
    tile_map[3 * 20 + 7] = '#';
    game_load(&blocked, &s);
    s.px = 1; s.py = 13;
    for (i = 0; i < 11; i++) game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 12 && s.guards[0].y == 3, "guard walked round the block (%d,%d)",
          s.guards[0].x, s.guards[0].y);
    CHECK(s.guards[0].next_wp == 0 && s.guards[0].facing == DIR_LEFT, "and turned for home");
    CHECK(s.status == STATUS_PLAYING, "player unseen");

    /* the walking distances are remembered between turns, but not across a
     * change to the map */
    tiles_reset(0, 0);
    game_load(&blocked, &s);
    s.px = 1; s.py = 13;
    for (i = 0; i < 9; i++) game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 12 && s.guards[0].y == 3, "straight there once it's clear");
}

static void test_tile_types(void)
{
    GameState s;
    const Guard *g;
    int i;

    /* door: walk through, can't see or throw through, hides you */
    tiles_reset('#', 'D');
    tiles_load(&s, 6, 8);
    g = &s.guards[0];
    CHECK(game_cell(7, 8) == CELL_DOOR, "door parsed");
    CHECK(game_guard_sees(g, 8, 8) && !game_guard_sees(g, 6, 8), "door blocks sight");
    CHECK(!game_can_throw(&s, ITEM_STONE, 8, 8), "door blocks throws");
    CHECK(game_step(&s, ACT_RIGHT) && s.px == 7, "walk into the doorway");
    CHECK(s.status == STATUS_PLAYING && !game_guard_sees(g, 7, 8),
          "standing in a doorway 3 tiles from a guard: hidden");

    /* window: see through, can't walk or throw through */
    tiles_reset('#', 'W');
    tiles_load(&s, 6, 8);
    CHECK(game_guard_sees(&s.guards[0], 6, 8), "window lets sight through");
    CHECK(!game_can_throw(&s, ITEM_STONE, 8, 8), "window blocks throws");
    CHECK(game_step(&s, ACT_RIGHT) == 0, "can't walk through a window");

    /* curtain: walk in, it hides you, throws pass */
    tiles_reset(0, 0);
    tile_map[8 * 20 + 8] = 'C';
    tiles_load(&s, 6, 8);
    g = &s.guards[0];
    CHECK(!game_guard_sees(g, 8, 8) && !game_guard_sees(g, 7, 8),
          "curtain hides what's in and behind it");
    CHECK(game_can_throw(&s, ITEM_STONE, 9, 8), "throws pass curtains");
    s.px = 8;
    CHECK(game_step(&s, ACT_WAIT) && s.status == STATUS_PLAYING, "hidden in the curtain");

    /* mesh: walk and see through, throws stop */
    tiles_reset(0, 0);
    tile_map[8 * 20 + 8] = 'M';
    tiles_load(&s, 6, 8);
    g = &s.guards[0];
    CHECK(game_guard_sees(g, 8, 8) && game_guard_sees(g, 7, 8), "mesh lets sight through");
    CHECK(!game_can_throw(&s, ITEM_STONE, 9, 8) && !game_can_throw(&s, ITEM_STONE, 8, 8),
          "mesh blocks throws (through it and onto it)");
    CHECK(game_can_throw(&s, ITEM_STONE, 7, 8), "but not short of it");

    /* railing: see and throw over it, nobody walks through; a guard sent to
     * a noise on the railing stops beside it and goes back */
    tiles_reset('R', 0);
    tiles_load(&s, 6, 3);
    g = &s.guards[0];
    CHECK(game_step(&s, ACT_RIGHT) == 0, "can't climb the railing");
    CHECK(game_can_throw(&s, ITEM_STONE, 9, 3), "throw over the railing");
    CHECK(game_throw(&s, ITEM_STONE, 7, 8), "throw onto the railing");
    CHECK(s.guards[0].mode == GUARD_INVESTIGATE, "guard heard it");
    for (i = 0; i < 3; i++) game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 8 && s.guards[0].y == 8, "guard stopped beside the railing");
    CHECK(s.guards[0].mode == GUARD_RETURN, "and gave up");
    for (i = 0; i < 3; i++) game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 10 && s.guards[0].mode == GUARD_PATROL, "then went back");
    CHECK(s.status == STATUS_PLAYING, "player unseen throughout");

    /* security door: guards walk through, the player can't, sight blocked */
    tiles_reset('#', 'S');
    tiles_load(&s, 6, 8);
    CHECK(game_step(&s, ACT_RIGHT) == 0, "player can't open a security door");
    CHECK(!game_guard_sees(&s.guards[0], 6, 8), "security door blocks sight");
    tiles_load(&s, 3, 5);
    CHECK(game_throw(&s, ITEM_STONE, 5, 8), "noise on the far side");
    for (i = 0; i < 8 && !(s.guards[0].x == 5 && s.guards[0].y == 8); i++)
        game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x == 5 && s.guards[0].y == 8, "guard came through the security door");
    CHECK(s.status == STATUS_PLAYING, "player unseen");
    /* a body lying in the security door can't be pushed: the player would
     * have to step into the doorway */
    tiles_load(&s, 6, 8);
    s.guards[0].mode = GUARD_BODY;
    s.guards[0].x = 7; s.guards[0].y = 8;
    CHECK(!game_can_drag(&s, DIR_RIGHT) && game_step(&s, ACT_DRAG + DIR_RIGHT) == 0 &&
          s.px == 6, "can't push a body through a security door");
    CHECK(game_step(&s, ACT_DRAG + DIR_LEFT) == 1 && s.guards[0].x == 6,
          "but can drag it out of the doorway");

    /* vent: the player crawls through, guards can't, it hides you */
    tiles_reset('V', 0);
    tiles_load(&s, 6, 8);
    g = &s.guards[0];
    CHECK(!game_guard_sees(g, 7, 8) && !game_guard_sees(g, 6, 8), "vents block sight");
    CHECK(game_step(&s, ACT_RIGHT) && s.px == 7, "player crawls into the vent");
    tiles_load(&s, 3, 5);
    CHECK(game_throw(&s, ITEM_STONE, 5, 8), "noise across the vent");
    for (i = 0; i < 6; i++) game_step(&s, ACT_WAIT);
    CHECK(s.guards[0].x >= 8, "guard can't get through the vent");
    CHECK(s.guards[0].mode == GUARD_PATROL && s.guards[0].x == 10, "and stays at its post");

    /* bodies move like the player: one can be pushed into a vent, where
     * it can't be seen */
    tiles_reset(0, 0);
    tile_map[8 * 20 + 8] = 'V';
    tiles_load(&s, 6, 8);
    s.guards[0].x = 15; s.guards[0].y = 8;   /* guard further right, facing left */
    s.guard_count = 2;
    s.guards[1].mode = GUARD_BODY;
    s.guards[1].x = 7; s.guards[1].y = 8;
    s.guards[1].facing = DIR_LEFT;
    s.px = 6; s.py = 8;
    CHECK(game_step(&s, ACT_DRAG + DIR_RIGHT) == 1, "push the body into the vent");
    CHECK(s.guards[1].x == 8 && s.status == STATUS_PLAYING,
          "body in the vent isn't spotted (guard 4 tiles away)");
}

/* Validate level data: sizes, starts and exits, links, guard routes.
 * levels[] has one entry per map; a level's maps are consecutive. */
static void test_level_data(void)
{
    int l, y, i, w, d, level_exits = 0;
    for (l = 0; l < LEVEL_COUNT; l++) {
        const LevelDef *lv = &levels[l];
        int players = 0, exits = 0;
        if (lv->part == 0) level_exits = 0;
        GameState s;
        size_t cells = (size_t)lv->width * lv->height;
        CHECK(lv->width >= VIEW_W && lv->width <= MAP_MAX_W &&
              lv->height >= VIEW_H && lv->height <= MAP_MAX_H &&
              cells <= MAP_MAX_CELLS && lv->width % VIEW_W == 0 &&
              lv->height % VIEW_H == 0, "level %d size %dx%d isn't 1-3 whole screens",
              l + 1, lv->width, lv->height);
        CHECK(strlen(lv->map) == cells, "level %d map has %zu chars, expected %zu",
              l + 1, strlen(lv->map), cells);
        for (y = 0; y < (int)cells; y++) {
            players += lv->map[y] == 'P';
            exits += lv->map[y] == 'E';
        }
        CHECK(players == (lv->part == 0), "map %d: the start (P) belongs in a level's first map",
              l + 1);
        level_exits += exits;
        /* characters: a look that exists; a prompt word with every script,
         * and nobody who talks can be taken down. Switch cells: on the map,
         * marked in the map with its own digit; one may be the level's exit */
        CHECK(lv->switch_count <= MAX_SWITCHES, "map %d: too many switch cells", l + 1);
        for (y = 0; y < lv->guard_count; y++) {
            const GuardDef *g = &lv->guards[y];
            CHECK(g->look < LOOK_COUNT, "map %d: character %d has no look", l + 1, y + 1);
            CHECK(!g->script == !g->verb && (!g->verb || strlen(g->verb) <= 8),
                  "map %d: character %d has a script but no prompt, or the reverse", l + 1, y + 1);
            CHECK(!g->script || (g->flags & CHAR_NO_TAKEDOWN),
                  "map %d: character %d talks but could be taken down", l + 1, y + 1);
        }
        for (y = 0; y < lv->switch_count; y++) {
            const SwitchDef *sw = &lv->switches[y];
            CHECK(sw->x < lv->width && sw->y < lv->height && sw->flag &&
                  lv->map[sw->y * lv->width + sw->x] == '0' + y,
                  "map %d: switch cell %d doesn't match the map", l + 1, y + 1);
            level_exits += sw->on == CELL_EXIT;
        }
        if (l + 1 == LEVEL_COUNT || levels[l + 1].part == 0)
            CHECK(level_exits >= 1, "level ending at map %d needs an exit", l + 1);
        CHECK(l == 0 || lv->part == 0 || lv->part == levels[l - 1].part + 1,
              "map %d: maps of a level must be consecutive", l + 1);
        for (d = 0; d < 4; d++) {
            int to = lv->link[d] - 1, back = (d + 2) % 4;
            if (!lv->link[d]) continue;
            CHECK(to >= 0 && to < LEVEL_COUNT && levels[to].link[back] == l + 1 &&
                  levels[to].link_offset[back] == -lv->link_offset[d],
                  "map %d: link %d must be matched by one back from map %d", l + 1, d, to + 1);
        }
        CHECK(strlen(lv->name) <= 12, "level %d name too long", l + 1);
        game_load(lv, &s);
        for (i = 0; i < lv->guard_count; i++) {
            const GuardDef *d = &lv->guards[i];
            CHECK(d->enforcer < (1 << DISGUISE_TYPES),
                  "level %d guard %d: enforcer is a mask of the disguises", l + 1, i);
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

static int solve(const LevelDef *table, int first, int use, char *out, int outlen)
{
    static SolveStep steps[1024];
    int states, len = solver_run(table, (uint8_t)first, use, steps, 1024, &states);
    if (getenv("SOLVER_STATS"))
        fprintf(stderr, "  [%s: %d states]\n", table[first].name, states);
    out[0] = 0;
    if (len > 0) solver_format(steps, len < 1024 ? len : 1024, out, outlen);
    return len;
}

/* Index in levels[] of the n-th level (1-based); levels can span several
 * entries, one per map. */
static int level_start(int n)
{
    int i;
    for (i = 0; i < LEVEL_COUNT; i++)
        if (levels[i].part == 0 && --n == 0) return i;
    return -1;
}

/* ---- Levels of several maps (world.c). Map A's right edge leads to map
 * B; B's row 0 lines up with A's row 2, so A's opening on rows 5-8 meets
 * B's on rows 3-6. ---- */
static LevelDef two_maps[2] = {
    {
        "TWO", 20, 16,
        "####################"
        "#..................#"
        "#.P................#"
        "#..................#"
        "#..................#"
        "#..................."
        "#..................."
        "#..................."
        "#..................."
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "####################",
        1,
        { { DIR_DOWN, 2, { {4, 10}, {14, 10} }, DISGUISE_NONE, 0 } },
        { 1, 0 },
        0, { 0, 2, 0, 0 }, { 0, 2, 0, 0 },      /* right -> B, offset 2 */
        TRANS_CUT, NO_SCRIPTING
    },
    {
        "TWO", 20, 16,
        "####################"
        "#..................#"
        "#..................#"
        "...................#"
        "...................#"
        "...................#"
        "...................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..................#"
        "#..............E...#"
        "#..................#"
        "#..................#"
        "####################",
        1,
        { { DIR_DOWN, 1, { {3, 10} }, DISGUISE_NONE, 0 } },
        { 0, 0 },
        1, { 0, 0, 0, 1 }, { 0, 0, 0, -2 },     /* left -> A, offset -2 */
        TRANS_CUT, NO_SCRIPTING
    },
};

static void test_world(void)
{
    World w, before;
    GameState *s;
    int i;
    char sol[1024];

    world_load(two_maps, 0, &w);
    CHECK(w.part_count == 2 && w.part_def[1] == 1, "linked map found");
    CHECK(w.cur == 0 && w.part[0].px == 2 && w.part[0].py == 2, "start in map A");
    CHECK(w.part[1].items[0] == 0 && w.part[0].items[0] == 1, "items start with the player");

    s = world_state(&w);
    s->px = 18; s->py = 6;
    CHECK(world_step(&w, ACT_RIGHT) == 1 && w.part[0].px == 19, "walk into the opening");
    CHECK(world_step(&w, ACT_DOWN) == 1, "move along it (row 7)");
    s->px = 19; s->py = 9;
    CHECK(world_step(&w, ACT_RIGHT) == 0, "no crossing where the edge is wall");
    s->py = 6;
    {
        uint8_t guard_x = w.part[0].guards[0].x, guard_y = w.part[0].guards[0].y;
        uint16_t turn = s->turn;
        undo_clear();
        before = w;
        undo_push(&w);
        CHECK(world_step(&w, ACT_RIGHT) == 1, "cross into map B");
        s = world_state(&w);
        CHECK(w.cur == 1 && s->px == 0 && s->py == 4, "arrive at row 6 - 2 = 4");
        CHECK(s->turn == turn + 1 && s->items[0] == 1, "turn and items carried over");
        CHECK(world_def(&w) == &two_maps[1] && game_width() == 20, "rules use map B");
        for (i = 0; i < 5; i++) { undo_push(&w); world_step(&w, ACT_WAIT); }
        CHECK(w.part[0].guards[0].x == guard_x && w.part[0].guards[0].y == guard_y,
              "map A's guard stands still while the player is away");
        CHECK(w.part[1].guards[0].facing == DIR_DOWN, "map B's guard keeps its post");
        for (i = 0; i < 5; i++) undo_pop(&w);
        CHECK(undo_pop(&w) && memcmp(&w, &before, WORLD_BYTES(&w)) == 0,
              "undo steps back across the crossing exactly");
    }

    /* cross and come back */
    world_step(&w, ACT_RIGHT);
    CHECK(world_step(&w, ACT_LEFT) == 1 && w.cur == 0 &&
          w.part[0].px == 19 && w.part[0].py == 6, "back into map A, same row");

    /* no dragging bodies across */
    world_load(two_maps, 0, &w);
    s = world_state(&w);
    s->px = 18; s->py = 6;
    s->guards[0].mode = GUARD_BODY; s->guards[0].x = 17; s->guards[0].y = 6;
    world_step(&w, ACT_RIGHT);
    CHECK(world_step(&w, ACT_DRAG + DIR_RIGHT) == 0 && w.cur == 0,
          "bodies stay in their map");

    /* a guard who can see the arrival spot catches the player */
    two_maps[1].guards[0].wp[0].x = 3; two_maps[1].guards[0].wp[0].y = 4;
    two_maps[1].guards[0].facing = DIR_LEFT;
    world_load(two_maps, 0, &w);
    w.part[0].px = 19; w.part[0].py = 6;
    world_step(&w, ACT_RIGHT);
    CHECK(w.cur == 1 && w.part[1].status == STATUS_CAUGHT, "spotted on arrival");
    two_maps[1].guards[0].wp[0].y = 10;
    two_maps[1].guards[0].facing = DIR_DOWN;

    /* the solver finds its way across (moves only: this is about crossing,
     * and two open rooms with every mechanic are a very big search) */
    CHECK(solve(two_maps, 0, 0, sol, sizeof sol) == 45,
          "two-map level: 20 to the edge, 1 to cross, 24 to the exit: %s", sol);
    two_maps[0].link[DIR_RIGHT] = 0;
    CHECK(solve(two_maps, 0, 0, sol, sizeof sol) == 0,
          "and can't be solved without the link");
    two_maps[0].link[DIR_RIGHT] = 2;
}

static void test_solvable(void)
{
    int l;
    char sol[1024];
    for (l = 1; level_start(l) >= 0; l++) {
        int i = level_start(l);
        int len = solve(levels, i, SOLVE_USE_ALL, sol, sizeof sol);
        CHECK(len > 0, "level %d (%s) is not solvable (%d)", l, levels[i].name, len);
        if (len > 0)
            printf("level %d %-12s solved in %3d turns: %s\n", l, levels[i].name, len, sol);
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
    { 3, SOLVE_USE_TALK,     "petting the dog" },
    { 5, SOLVE_USE_TAKEDOWN, "a takedown" },
    { 5, SOLVE_USE_DRAG,     "dragging the body" },
    { 6, SOLVE_USE_UNIFORM,  "a disguise" },
    /* level 7: the stone lures the shaft sentry; or take the corridor
     * guard's uniform, which the sentry is blind to */
    { 7, SOLVE_USE_ITEMS | SOLVE_USE_UNIFORM, "the stone or a disguise" },
};

/* Levels built around a tile must need it: with every such tile turned
 * into wall, the solver must fail. */
static const struct {
    int  level;        /* 1-based */
    char tile;
    const char *what;
} required_tiles[] = {
    { 8, 'V', "the vent" },
};

static void test_tiles_needed(void)
{
    static char map[MAP_MAX_CELLS + 1];
    unsigned r;
    size_t i;
    char sol[1024];
    for (r = 0; r < sizeof required_tiles / sizeof required_tiles[0]; r++) {
        LevelDef copy = levels[level_start(required_tiles[r].level)];
        for (i = 0; copy.map[i]; i++)
            map[i] = copy.map[i] == required_tiles[r].tile ? '#' : copy.map[i];
        map[i] = 0;
        copy.map = map;
        CHECK(solve(&copy, 0, SOLVE_USE_ALL, sol, sizeof sol) == 0,
              "level %d (%s) can be beaten without %s: %s",
              required_tiles[r].level, copy.name, required_tiles[r].what, sol);
    }
}

static void test_mechanics_needed(void)
{
    unsigned r;
    char sol[1024];
    for (r = 0; r < sizeof required / sizeof required[0]; r++) {
        int first = level_start(required[r].level);
        const LevelDef *lv = &levels[first];
        CHECK(solve(levels, first, SOLVE_USE_ALL & ~required[r].without, sol, sizeof sol) == 0,
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
    test_access();
    test_big_map();
    test_tile_types();
    test_patrol_detour();
    test_transitions();
    test_npcs();
    test_walking_talker();
    test_level_data();
    if (failures == 0) {
        test_world();
        test_solvable();
        test_mechanics_needed();
        test_tiles_needed();
    }

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
