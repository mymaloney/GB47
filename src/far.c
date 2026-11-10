/*
 * far.c - See far.h. Must stay in bank 0 (no "#pragma bank").
 */
#include <string.h>
#include "far.h"

#ifdef __SDCC
#include <gb/gb.h>

void far_read(uint8_t bank, void *dst, const void *src, uint16_t n)
{
    const uint8_t *s = src;
    uint8_t       *d = dst;

    SWITCH_ROM(bank);
    while (n--) *d++ = *s++;
    SWITCH_ROM(BANK_CODE);
}

#else

void far_read(uint8_t bank, void *dst, const void *src, uint16_t n)
{
    (void)bank;
    memcpy(dst, src, n);
}

#endif
