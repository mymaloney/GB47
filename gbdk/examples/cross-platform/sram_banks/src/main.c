#include <gbdk/platform.h>
#include <stdint.h>
#include <stdio.h>

extern int sram_bank_var_0;  /* In Cart SRAM bank 0 */
extern int sram_bank_var_1;  /* In Cart SRAM bank 1 */

void main(void)
{
  puts("Vars in \n"
       "Cart SRAM Banks\n\n"
       "Assigned using\n\n"
       " #pragma\n"
       " dataseg DATA_<n>\n"
       );

  ENABLE_RAM;

  SWITCH_RAM(0);
  sram_bank_var_0 = 0;
  SWITCH_RAM(1);
  sram_bank_var_1 = 1;
  SWITCH_RAM(2);

  SWITCH_RAM(0);
  printf("sram_bank_var0 is %u\n", sram_bank_var_0);
  
  SWITCH_RAM(1);
  printf("sram_bank_var1 is %u\n", sram_bank_var_1);
}
