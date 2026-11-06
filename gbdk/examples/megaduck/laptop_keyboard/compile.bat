REM Automatically generated from Makefile

..\..\..\\bin\lcc -Wf-MMD -Wf-Wp-MP  -msm83:duck -c -o obj\duck\main.o src\main.c
..\..\..\\bin\lcc -Wf-MMD -Wf-Wp-MP  -msm83:duck -c -o obj\duck\megaduck_key2ascii.o src\megaduck_key2ascii.c
..\..\..\\bin\lcc -Wf-MMD -Wf-Wp-MP  -msm83:duck -c -o obj\duck\megaduck_keyboard.o src\megaduck_keyboard.c
..\..\..\\bin\lcc   -Wl-j  -Wf-MMD -Wf-Wp-MP  -Wf-MMD -Wf-Wp-MP  -msm83:duck -o build\duck\megaduck_keyboard.duck obj\duck\main.o obj\duck\megaduck_key2ascii.o obj\duck\megaduck_keyboard.o 
