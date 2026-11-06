/*
 * levels.c - Level data.
 *
 * Map legend:  '#' wall   '.' floor   'E' exit   'P' player start
 * Guards walk a loop through their waypoints, one tile per turn. Each leg
 * must be a straight line (same row or same column as the previous point).
 * A guard with a single waypoint is a stationary sentry.
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
            { DIR_RIGHT, 2, { {1, 5}, {18, 5} } },
        },
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
            { DIR_DOWN, 3, { {7, 4},  {7, 13}, {7, 1} } },
            { DIR_UP,   3, { {11, 10}, {11, 1}, {11, 13} } },
        },
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
            { DIR_LEFT, 1, { {14, 10} } },
            { DIR_DOWN, 4, { {6, 1}, {6, 9}, {18, 9}, {18, 1} } },
        },
    },
};
