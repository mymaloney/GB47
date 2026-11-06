#include <gbdk/platform.h>
#include <stdio.h>
#include <stdbool.h>

volatile bool PauseRequested = false; 

void NMI_ISR (void) CRITICAL INTERRUPT {   // the function name MUST be "NMI_ISR"
    PauseRequested = true;
}

void main(void) {
    puts("Press PAUSE button");
    while(1) {
        if (PauseRequested) {
            puts("PAUSED! press A to unpause");
            waitpad(J_A);
            puts("UNPAUSED");            
            PauseRequested = false;
        }
        wait_vbl_done();
    } 
}
