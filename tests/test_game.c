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
#include "../src/world.h"

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
        { DIR_RIGHT, 1, { {2, 3} },           DISGUISE_GUARD, "HBE" },
        { DIR_RIGHT, 2, { {2, 6}, {17, 6} },  DISGUISE_GUARD, "EBE" },
    },
    { 0, 0 },
    0, { 0 }, { 0 },   /* single map: no links */
};

static void test_disguise_cones(void)
{
    static uint8_t vis[VIEW_H][VIEW_W];
    GameState s;

    game_load(&levels[0], &s);
    CHECK(game_attitude(&s, 0) == ATT_ENFORCER, "no attitude string = enforcer");

    /* hostile: 7 deep, up to 7 wide */
    game_load(&dz_level, &s);
    CHECK(game_attitude(&s, 0) == ATT_HOSTILE, "hostile to own clothes");
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
    CHECK(game_attitude(&s, 0) == ATT_BLIND, "blind to GUARD");
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
    test_big_map();
    test_tile_types();
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
