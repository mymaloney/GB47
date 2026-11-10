/*
 * art.c - Painted art layer helpers. See art.h.
 */
#include "art.h"

void art_row(const MapArt *a, uint8_t y, uint8_t x0, uint8_t w, uint8_t *out)
{
    const uint8_t *p = a->rle + a->rows[y];
    uint8_t x = 0, run, v, i;

    /* skip runs that end before x0 */
    for (;;) {
        run = p[0];
        if ((uint16_t)x + run > x0) break;
        x += run;
        p += 2;
    }
    /* copy from the run that contains x0 */
    i = 0;
    run -= x0 - x;
    while (i < w) {
        v = p[1];
        while (run && i < w) {
            out[i++] = v;
            run--;
        }
        p += 2;
        run = p[0];
    }
}

/* Where a blind guard's watch area lightens a tile (as TILE_WATCH). */
static const uint8_t watch_dots[8] = { 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00, 0x11 };

/* Pixel colour = lo bit + 2 * hi bit; colour 0 is the lightest. */
void art_variant(const uint8_t *src, uint8_t variant, uint8_t *dst)
{
    uint8_t y, lo, hi, blank;
    for (y = 0; y < 8; y++) {
        lo = src[2 * y];
        hi = src[2 * y + 1];
        blank = (uint8_t)~(lo | hi);               /* colour-0 pixels */
        if (variant == ART_SEEN || variant == ART_AIM_SEEN) lo |= blank;   /* -> 1 */
        if (variant == ART_WATCH) lo |= blank & watch_dots[y];
        if ((variant == ART_AIM || variant == ART_AIM_SEEN) && (y == 3 || y == 4)) {
            hi |= 0x18;                            /* the target dot */
            if (variant == ART_AIM) lo &= (uint8_t)~0x18;   /* colour 2 */
            else lo |= 0x18;                               /* colour 3 */
        }
        dst[2 * y] = lo;
        dst[2 * y + 1] = hi;
    }
}
