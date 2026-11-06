#include <gbdk/platform.h>
#include <stdint.h>
#include <stdio.h>

#include "hram_data.h"

void main(void) {
    my_hram_variable = 5;
    printf("value is: %hd at %x", (uint8_t)my_hram_variable, (uint16_t)&my_hram_variable);  
}