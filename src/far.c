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

#define RLE_BANK_IN(bank) SWITCH_ROM(bank)
#define RLE_BANK_OUT()    SWITCH_ROM(BANK_CODE)

#else

void far_read(uint8_t bank, void *dst, const void *src, uint16_t n)
{
    (void)bank;
    memcpy(dst, src, n);
}

#define RLE_BANK_IN(bank) (void)(bank)
#define RLE_BANK_OUT()

#endif

void far_read_rle(uint8_t bank, uint8_t *dst, const uint8_t *src, uint16_t n)
{
    uint8_t run, v;

    RLE_BANK_IN(bank);
    while (n) {
        run = *src++;
        v   = *src++;
        if (!run || run > n) break;     /* bad data: stop rather than overrun */
        n -= run;
        while (run--) *dst++ = v;
    }
    RLE_BANK_OUT();
}
