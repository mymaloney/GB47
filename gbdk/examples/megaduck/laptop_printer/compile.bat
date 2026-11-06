REM Automatically generated from Makefile

..\..\..\\bin\lcc -Wf-MMD -Wf-Wp-MP  -msm83:duck -c -o obj\duck\main.o src\main.c
..\..\..\\bin\lcc -Wf-MMD -Wf-Wp-MP  -msm83:duck -c -o obj\duck\megaduck_printer.o src\megaduck_printer.c
..\..\..\\bin\lcc -Wf-MMD -Wf-Wp-MP  -msm83:duck -c -o obj\duck\megaduck_printscreen.o src\megaduck_printscreen.c
..\..\..\\bin\lcc   -Wl-j  -Wf-MMD -Wf-Wp-MP  -Wf-MMD -Wf-Wp-MP  -msm83:duck -o build\duck\megaduck_printer.duck obj\duck\main.o obj\duck\megaduck_printer.o obj\duck\megaduck_printscreen.o 
