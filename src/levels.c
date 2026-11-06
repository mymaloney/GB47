/*
 * levels.c - Level data.
 *
 * Map legend:  '#' wall   '.' floor   'E' exit   'P' player start
 * Guards walk a loop through their waypoints, one tile per turn. Each leg
 * must be a straight line (same row or same column as the previous point).
 * A guard with a single waypoint is a stationary sentry.
 *
 * After the waypoints: the uniform the guard wears (what the player gets
 * from its body; DISGUISE_NONE = none) and its attitude to each disguise
 * as a string indexed by DISGUISE_*: 'H' hostile, 'E' enforcer, 'B' blind
 * (0 = enforcer to everything). See game.h.
 *
 * Items: { stones, bottles } the player starts with.
 *
 * Each row must be exactly 20 characters. tests/test_game.c checks that
 * every level is well formed and solvable.
 *
 * Later this will come from Tiled exports; ASCII is fine for prototyping.
 */
#include "levels.h"

const LevelDef levels[LEVEL_COUNT] = {
    /* 1: Learn that guards only move when you do. Follow one through. */
    {
        "FIRST STEPS",
        {
            "####################",
            "#P.....#############",
            "#......#############",
            "#......#############",
            "###.################",
            "#..................#",
            "#..................#",
            "############.#######",
            "##########.....#####",
            "##########...E.#####",
            "##########.....#####",
            "####################",
            "####################",
            "####################",
            "####################",
            "####################",
        },
        1,
        {
            { DIR_RIGHT, 2, { {1, 5}, {18, 5} }, DISGUISE_NONE, 0 },
        },
        { 0, 0 },
    },

    /* 2: Two patrols crossing a hall. Use the pillars as cover. */
    {
        "CROSSFIRE",
        {
            "####################",
            "#..................#",
            "#..................#",
            "#...##.......##....#",
            "#...##.......##....#",
            "#..................#",
            "#..................#",
            "#P................E#",
            "#..................#",
            "#..................#",
            "#...##.......##....#",
            "#...##.......##....#",
            "#..................#",
            "#..................#",
            "####################",
            "####################",
        },
        2,
        {
            { DIR_DOWN, 3, { {7, 4},  {7, 13}, {7, 1} }, DISGUISE_NONE, 0 },
            { DIR_UP,   3, { {11, 10}, {11, 1}, {11, 13} }, DISGUISE_NONE, 0 },
        },
        { 0, 0 },
    },

    /* 3: A sentry watches the door; wait for the patrol to pass. */
    {
        "THE SENTRY",
        {
            "####################",
            "#P.#...............#",
            "#..#...............#",
            "#..#....######.....#",
            "#..#....#....#.....#",
            "#.......#....#.....#",
            "#..#....#....#.....#",
            "#..#....##.###.....#",
            "#..#...............#",
            "####...............#",
            "#..................#",
            "#..######.##########",
            "#..#...............#",
            "#..#............E..#",
            "#..#...............#",
            "####################",
        },
        2,
        {
            { DIR_LEFT, 1, { {14, 10} }, DISGUISE_NONE, 0 },
            { DIR_DOWN, 4, { {6, 1}, {6, 9}, {18, 9}, {18, 1} }, DISGUISE_NONE, 0 },
        },
        { 0, 0 },
    },

    /* 4: The sentry never leaves the door... unless it hears something.
     * SELECT opens the inventory; throw to lure it out, then slip past. */
    {
        "LURE",
        {
            "####################",
            "#..................#",
            "#..................#",
            "#..................#",
            "#..........#########",
            "#..........#.......#",
            "#P................E#",
            "#..........#.......#",
            "#..........#########",
            "#..................#",
            "#..................#",
            "#..................#",
            "####################",
            "####################",
            "####################",
            "####################",
        },
        1,
        {
            { DIR_LEFT, 1, { {12, 6} }, DISGUISE_NONE, 0 },
        },
        { 1, 1 },   /* one stone, one bottle */
    },

    /* 5: The doorway guard faces away: take him down from behind (B), then
     * hold B and drag the body out of the patrol's sight. */
    {
        "CLEANUP",
        {
            "####################",
            "#.................E#",
            "#..................#",
            "#########.##########",
            "#########.##########",
            "#########.##########",
            "#####...........####",
            "#####..P........####",
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
            { DIR_UP,    1, { {9, 3} }, DISGUISE_NONE, 0 },
            { DIR_RIGHT, 2, { {5, 2}, {13, 2} }, DISGUISE_NONE, 0 },
        },
        { 0, 0 },
    },

    /* 6: The hall sentry is hostile to your clothes (huge cone) but blind
     * to his own uniform. Take one from the lone guard, then get past the
     * officer, who still checks guards. */
    {
        "DRESS CODE",
        {
            "####################",
            "#P.......#........E#",
            "#........#.........#",
            "#........#.........#",
            "#..................#",
            "#........#.........#",
            "#........#.........#",
            "#........#.........#",
            "####################",
            "####################",
            "####################",
            "####################",
            "####################",
            "####################",
            "####################",
            "####################",
        },
        3,
        {
            /*                                        attitude to: own clothes,
                                                      GUARD, OFFICER         */
            { DIR_RIGHT, 2, { {2, 6}, {7, 6} },  DISGUISE_GUARD,   "EBE" },
            { DIR_LEFT,  1, { {17, 4} },         DISGUISE_GUARD,   "HBE" },
            { DIR_DOWN,  2, { {14, 1}, {14, 7} }, DISGUISE_OFFICER, "HEB" },
        },
        { 0, 0 },
    },
};
