#include <gbdk/platform.h>
#include <gbdk/console.h>
#include <gb/isr.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <duck/laptop_io.h>

#include "megaduck_printer.h"
#include "megaduck_printscreen.h"

uint8_t printer_type;

const uint8_t bg_tile[] = {
    0x00, 0x00,
    0x00, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0x00, 0xFF,
    0x00, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF, };


uint8_t printer_query(void) {

     printf("Printer Query\n");

    uint8_t printer_query_result = duck_io_printer_query();
    printf("Printer Result %hx\n", printer_query_result);

    // return (printer_query_result & DUCK_IO_PRINTER_INIT_OK);
    return printer_query_result;
}


static bool duck_laptop_and_printer_init(void) {

    bool megaduck_laptop_init_ok = duck_io_laptop_init();
    if (!megaduck_laptop_init_ok) {
        // If laptop hardware is not present then there isn't anything
        // for this program to do, so just idle in a loop
        printf("Laptop not detected\n"
               "or Failed to Initialize\n");
        return false;
    }

    // Otherwise laptop init succeeded
    printf("Laptop Detected!\n");

    // Check to see if the printer was connected at startup, and if so what model
    printer_type = duck_io_printer_last_status();
    // Fix up printer status == 3 to be printer type 1, sort of a hack
    if (printer_type == DUCK_IO_PRINTER_MAYBE_BUSY) printer_type = DUCK_IO_PRINTER_TYPE_1_PASS;

    // If that failed, try a re-query of the printer    
    if (printer_type == DUCK_IO_PRINTER_FAIL) {

        printf("No printer at startup\n"
               " re-querying...\n");
        if (duck_io_printer_query() == DUCK_IO_PRINTER_FAIL) {
            printf("Printer not found\n");
            return false;
        }
    }

    // Otherwise printer init succeeded
    printf("Printer Detected!\n");
    if (printer_type == DUCK_IO_PRINTER_TYPE_1_PASS)
        printf("- Single Pass model\n");
    else
        printf("- Double Pass model\n");

    return true;    
}


void main(void) {

    uint8_t gamepad = 0;
    uint8_t gamepad_last = 0;

    SPRITES_8x8;
    SHOW_SPRITES;
    SHOW_BKG;
    printf("Initializing..\n");

    // Set tile as last tile
    set_bkg_data(255, 1, bg_tile);    
    fill_bkg_rect(0, DEVICE_SCREEN_HEIGHT - 4, DEVICE_SCREEN_WIDTH - 1, DEVICE_SCREEN_HEIGHT - 1, 255);

    // Stop here if no printer detected
    if (duck_laptop_and_printer_init() == false) {        
        // Optionally take action if no printer is detected
    }

    printf("\nPress:\n"
           "* START to print\n"
           "* SELECT requery\n"
           "* A print blank row\n");

	while(1) {
	    vsync();
        gamepad_last = gamepad;
        gamepad = joypad();

        // Send command to print the screen if START is pressed
        switch (gamepad) {
            case J_START:
                printf("Starting print...\n");
                // uint8_t printer_type = printer_query();
                bool print_job_status = duck_print_screen();
                printf("Finished print, status: %hu\n", print_job_status);
                // Wait until START is released before continuing
                waitpadup();
                break;
            case J_A: duck_print_blank_row(); waitpadup(); break;
            case J_SELECT: printer_query(); waitpadup(); break;
        }
	}
}
