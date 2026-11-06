#ifndef _MBC5_RUMBLE_H
#define _MBC5_RUMBLE_H

#include <gbdk/platform.h>
#include <stdint.h>

#define RUMBLE_FRAMES(N) (N + 1u)
#define RUMBLE_COUNT_1_SEC       RUMBLE_FRAMES(60u)
#define RUMBLE_COUNT_HALF_SEC    RUMBLE_FRAMES(30u)
#define RUMBLE_COUNT_QUARTER_SEC RUMBLE_FRAMES(15u)
#define RUMBLE_COUNT_EIGHTH_SEC  RUMBLE_FRAMES(7u)
#define RUMBLE_COUNT_DONE        0u


// MAX uses an always on duty cycle.
// The others use the various waveforms below
//
// Using shorter on/off cycling makes
// the rumble feel less "lumpy" than longer
// contiguous on-off square waves with
// the same amount of total on-time frames.
//                                              // The counter increments once per frame
//                                              // ("Y" = on, "-" = off)
//                                              // Bit 76543210    7 6 5 4 3 2 1 0
#define RUMBLE_DUTY_MASK_MAX     (0b11111111u)  //     YYYYYYYY   |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
#define RUMBLE_DUTY_MASK_HI      (0b00000011u)  //     YYY-YYY-   |¯¯¯¯¯|_|¯¯¯¯¯|_|
#define RUMBLE_DUTY_MASK_MED     (0b00000010u)  //     YY--YY--   |¯¯¯|___|¯¯¯|___|
#define RUMBLE_DUTY_MASK_LOW     (0b00000001u)  //     Y-Y-Y-Y-   |¯|_|¯|_|¯|_|¯|_|

#define RUMBLE_INTENSITY_MAX     3u
#define RUMBLE_INTENSITY_HI      2u
#define RUMBLE_INTENSITY_MED     1u
#define RUMBLE_INTENSITY_LOW     0u
#define RUMBLE_INTENSITY_MIN     (RUMBLE_INTENSITY_LOW)

#define MBC_RAM_BANK_0           0u

#define MBC_RUMBLE_BIT_ON        0b00001000u
#define MBC_RUMBLE_BIT_OFF       0b00000000u
#define MBC5_RUMBLE_ON           SWITCH_RAM_MBC5(MBC_RAM_BANK_0 | MBC_RUMBLE_BIT_ON)
#define MBC5_RUMBLE_OFF          SWITCH_RAM_MBC5(MBC_RAM_BANK_0 | MBC_RUMBLE_BIT_OFF)


void rumble_start(uint8_t duration);
void rumble_cancel(void);

void rumble_intensity_set(uint8_t duty_cycle);
uint8_t rumble_intensity_increase(void);
uint8_t rumble_intensity_decrease(void);

void rumble_update(void);

#endif // _MBC5_RUMBLE_H
