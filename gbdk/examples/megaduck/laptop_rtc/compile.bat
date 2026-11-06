REM Automatically generated from Makefile

..\..\..\\bin\lcc -Wf-MMD -Wf-Wp-MP  -msm83:duck -c -o obj\duck\main.o src\main.c
..\..\..\\bin\lcc -Wf-MMD -Wf-Wp-MP  -msm83:duck -c -o obj\duck\megaduck_rtc.o src\megaduck_rtc.c
..\..\..\\bin\lcc   -Wl-j  -Wf-MMD -Wf-Wp-MP  -Wf-MMD -Wf-Wp-MP  -msm83:duck -o build\duck\megaduck_rtc.duck obj\duck\main.o obj\duck\megaduck_rtc.o 
