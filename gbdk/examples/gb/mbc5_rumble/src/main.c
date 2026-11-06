#include <gbdk/platform.h>
#include <stdint.h>
#include <stdio.h>
#include <gbdk/console.h>

#include "mbc5_rumble.h"

// Note about SRAM Banks
//
// If rumble is used in a program which does cart SRAM bank switching then the
// SRAM bank should be kept track of in a variable. The MBC5_RUMBLE_ON and
// MBC5_RUMBLE_OFF macros should be changed to include that SRAM bank variable
// (replacing MBC_RAM_BANK_0) so that rumble updates do not unexpectedly alter
// the cart SRAM bank.
// 
// Likewise when switching cart SRAM banks to access data, the rumble state
// should be included when calling SWITCH_RAM() or SWITCH_RAM_MBC5() so
// that rumble status is not interrupted when active.


// New and previous values of the joypad input
uint8_t joy  = 0, old_joy;
// User input macros
#define INPUT_UPDATE               (old_joy=joy,joy=joypad())
#define INPUT_BUTTON(key)          (joy&(key))
#define INPUT_BUTTONPRESS(key)     ((joy & ~old_joy) & (key))
#define INPUT_BUTTONS_PRESSED      (joy & ~old_joy)


#define INTENS_PRNT_X  18u
#define INTENS_PRNT_Y  12u

static void display_rumble_intensity(uint8_t value) {
    gotoxy(INTENS_PRNT_X, INTENS_PRNT_Y);
    printf("%hu", (uint8_t)value);
}


void main(void)
{
    // Install Rumble update function as aV-Blank handler
    CRITICAL {
        add_VBL(rumble_update);
    }

    printf(
        "MBC5 Rumble Example\n"
        "\n"
        "Press:\n"
        "-ST for 1/2 second\n"
        "-A  for 1/4 second\n"
        "-B  for 1/8 second\n"
        "\n"
        "-SEL to stop early\n"
        "\n"
        "-Up more intensity\n"
        "-Dn less intensity\n"
        "\n"
        "Rumble intensity:");

    // Set the rumble intensity to MAX
    rumble_intensity_set(RUMBLE_INTENSITY_MAX);
    display_rumble_intensity(RUMBLE_INTENSITY_MAX);

    INPUT_UPDATE;
    while(1) {

        INPUT_UPDATE;

        switch(INPUT_BUTTONS_PRESSED) {
            case J_START:  rumble_start(RUMBLE_COUNT_HALF_SEC); break;
            case J_A:      rumble_start(RUMBLE_COUNT_QUARTER_SEC); break;
            case J_B:      rumble_start(RUMBLE_COUNT_EIGHTH_SEC); break;

            case J_UP:     display_rumble_intensity( rumble_intensity_increase() );
                           break;
            case J_DOWN:   display_rumble_intensity( rumble_intensity_decrease() );
                           break;

            case J_SELECT: rumble_cancel(); break;
        }

        vsync();
    }
}
