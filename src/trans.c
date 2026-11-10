/*
 * trans.c - Level transition patterns. See trans.h.
 */
#include "trans.h"

static const uint8_t steps[TRANS_COUNT] = {
    0, 0, TRANS_W, TRANS_W / 2, 0 /* iris: see trans_steps */, 16,
    (TRANS_W + TRANS_H) / 2, 6 * 3, TRANS_W,
};

/* 4x4 ordered-dither matrix: neighbouring tiles go at very different times. */
static const uint8_t bayer[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5,
};

/* Which ring of the iris (x, y) is in. Roughly round: the longer distance
 * to the centre plus half the shorter, in rings two tiles wide. */
static uint8_t iris_ring(uint8_t x, uint8_t y, uint8_t cx, uint8_t cy)
{
    uint8_t a = x > cx ? x - cx : cx - x;
    uint8_t b = y > cy ? y - cy : cy - y;
    return (uint8_t)(a > b ? a + (b >> 1) : b + (a >> 1)) >> 1;
}

uint8_t trans_steps(uint8_t kind, uint8_t cx, uint8_t cy)
{
    uint8_t x, y, r, far = 0;

    if (kind >= TRANS_COUNT) return 0;
    if (kind != TRANS_IRIS) return steps[kind];
    /* as many rings as it takes to reach the furthest corner */
    for (y = 0; y < TRANS_H; y += TRANS_H - 1)
        for (x = 0; x < TRANS_W; x += TRANS_W - 1) {
            r = iris_ring(x, y, cx, cy);
            if (r > far) far = r;
        }
    return far + 1;
}

uint8_t trans_phase(uint8_t kind, uint8_t in, uint8_t n, uint8_t x, uint8_t y,
                    uint8_t cx, uint8_t cy)
{
    uint8_t a;

    switch (kind) {
    case TRANS_WIPE:
        return x;
    case TRANS_CURTAIN:                 /* distance from the nearer side */
        a = x < TRANS_W / 2 ? x : TRANS_W - 1 - x;
        return in ? n - 1 - a : a;
    case TRANS_IRIS:                    /* far tiles go first, near ones return first */
        a = iris_ring(x, y, cx, cy);
        return in ? a : n - 1 - a;
    case TRANS_DISSOLVE:
        return bayer[((y & 3) << 2) | (x & 3)];
    case TRANS_DIAGONAL:
        return (uint8_t)(x + y) >> 1;
    case TRANS_BLINDS:
        /* three slats of six rows; each row of a slat draws left to right
         * in three pieces */
        while (y >= 6) y -= 6;
        return (uint8_t)(y * 3) + (x >> 3);
    case TRANS_INTERLACE:
        return (y & 1) ? TRANS_W - 1 - x : x;
    default:
        return 0;
    }
}
