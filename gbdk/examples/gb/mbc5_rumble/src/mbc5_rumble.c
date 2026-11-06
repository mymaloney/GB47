#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>
#include "mbc5_rumble.h"

// Power up defaults
uint8_t rumble_count = 0u;
uint8_t rumble_intensity = RUMBLE_INTENSITY_MAX;
uint8_t rumble_mask      = RUMBLE_DUTY_MASK_MAX;

// Lookup table for bitwise duty cycle masks based on intensity setting
const uint8_t rumble_masks[RUMBLE_INTENSITY_MAX + 1] = {
    RUMBLE_DUTY_MASK_LOW,
    RUMBLE_DUTY_MASK_MED,
    RUMBLE_DUTY_MASK_HI,
    RUMBLE_DUTY_MASK_MAX,
};


void rumble_start(uint8_t duration) {
    rumble_count = duration;
}


void rumble_cancel(void) {
    rumble_count = RUMBLE_COUNT_DONE;
    MBC5_RUMBLE_OFF;
}


void rumble_set_duty_cycle(void) {
    rumble_mask = rumble_masks[rumble_intensity];
}


void rumble_intensity_set(uint8_t intensity) {    

    if (intensity > RUMBLE_INTENSITY_MAX)
        intensity = RUMBLE_INTENSITY_MAX;
    rumble_intensity = intensity;
    rumble_set_duty_cycle();
}


uint8_t rumble_intensity_increase(void) {

    if (rumble_intensity <  RUMBLE_INTENSITY_MAX)
        rumble_intensity++;
    rumble_set_duty_cycle();

    return rumble_intensity;
}


uint8_t rumble_intensity_decrease(void) {

    if (rumble_intensity > RUMBLE_INTENSITY_MIN)
        rumble_intensity--;
    rumble_set_duty_cycle();

    return rumble_intensity;
}


// Update cart rumble, should be called once per frame such as in a VBlank ISR
void rumble_update(void) {

    if (rumble_count) {
        rumble_count--;

        // For max intensity do a 100% duty cycle (always on).
        // For lower intensities use bit masked partial duty cycles.
        //
        // When the counter reaches the end (zero) it will get turned
        // off due to no bits matching any of the masks
        if (rumble_count & rumble_mask)
            MBC5_RUMBLE_ON;
        else
            MBC5_RUMBLE_OFF;
    }
}
