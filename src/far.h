/*
 * far.h - Reading level data that lives in switchable ROM banks.
 *
 * The cartridge is MBC5 with RAM. Its ROM is laid out as:
 *
 *   bank 0   0x0000-0x3FFF  the rules (game.c, world.c), art.c, trans.c,
 *                           far.c and the GBDK library. Always there.
 *   bank 1   0x4000-0x7FFF  main.c, the built-in tiles, and the level
 *                           table (levels.c: LevelDefs, names, scripts,
 *                           text). Mapped all the time, except inside the
 *                           two functions below.
 *   BANK_MAPS, BANK_ART     the big data: map cells, and painted art
 *                           (levels_maps.c, levels_art.c). Mapped in place
 *                           of bank 1 only while being copied out.
 *
 * So ordinary code never thinks about banks: it reads the level table
 * directly, and gets map cells and art through far_read() / art_row(),
 * which copy into RAM. Only code in bank 0 may switch banks (anything in
 * bank 1 would pull the rug from under itself): that is far.c and art.c.
 *
 * On a PC (tests, the editor's library) there are no banks: far_read is
 * a plain copy.
 */
#ifndef FAR_H
#define FAR_H

#include <stdint.h>

#define BANK_CODE 1   /* what's normally mapped at 0x4000 */
#define BANK_MAPS 2
#define BANK_ART  3

/* Copy n bytes at src, which is in ROM bank `bank`, to dst (in RAM). */
void far_read(uint8_t bank, void *dst, const void *src, uint16_t n);

/* The same for run-length data: src is (run, byte) pairs that unpack to
 * exactly n bytes. */
void far_read_rle(uint8_t bank, uint8_t *dst, const uint8_t *src, uint16_t n);

#endif
