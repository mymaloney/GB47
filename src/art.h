/*
 * art.h - Painted art layer for maps (tiles from a tileset).
 *
 * A map's art says which tileset tile to draw on each cell; cells without
 * art use the built-in graphics for their kind (wall, floor, ...). Art is
 * only looks: the rules never read it.
 *
 * Each map lists the tileset tiles it uses ("local" tiles 0..count-1) and
 * how many VRAM slots each needs (kinds[]): 1 = just the tile; 3 = plus
 * cone variants (seen, watched); 5 = plus aiming variants (aim, aim+seen).
 * The art itself is run-length encoded per row: (run, local tile) byte
 * pairs, ART_NONE for "no art here".
 */
#ifndef ART_H
#define ART_H

#include <stdint.h>

#define ART_NONE 0xFF

/* Variants, in the order their VRAM slots follow the base tile. */
#define ART_BASE_TILE 0
#define ART_SEEN      1
#define ART_WATCH     2
#define ART_AIM       3
#define ART_AIM_SEEN  4

typedef struct {
    uint8_t         count;   /* local tiles */
    const uint8_t  *tiles;   /* tileset tile for each (index into art_tile_data) */
    const uint8_t  *kinds;   /* VRAM slots each needs: 1, 3 or 5 */
    const uint8_t  *rle;     /* (run, local tile) pairs, row after row */
    const uint16_t *rows;    /* where each row starts in rle */
} MapArt;

/* Local tiles of row y, columns x0 .. x0+w-1, into out (ART_NONE = none). */
void art_row(const MapArt *a, uint8_t y, uint8_t x0, uint8_t w, uint8_t *out);

/* Make variant (ART_*) of the 16-byte tile src into dst. Must match
 * variant() in tools/tileset.py. */
void art_variant(const uint8_t *src, uint8_t variant, uint8_t *dst);

#endif
