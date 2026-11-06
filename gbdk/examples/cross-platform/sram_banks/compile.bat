REM Automatically generated from Makefile

..\..\..\bin\lcc -Wl-yt0x1B   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -msm83:gb -c -o obj\gb\main.o src\main.c
..\..\..\bin\lcc -Wl-yt0x1B   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -msm83:gb -c -o obj\gb\sram_bank0.o src\sram_bank0.c
..\..\..\bin\lcc -Wl-yt0x1B   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -msm83:gb -c -o obj\gb\sram_bank1.o src\sram_bank1.c
..\..\..\bin\lcc -Wl-yt0x1B   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -msm83:gb -o build\gb\sram_banks.gb obj\gb\main.o obj\gb\sram_bank0.o obj\gb\sram_bank1.o 

..\..\..\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -mz80:sms -c -o obj\sms\main.o src\main.c
..\..\..\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -mz80:sms -c -o obj\sms\sram_bank0.o src\sram_bank0.c
..\..\..\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -mz80:sms -c -o obj\sms\sram_bank1.o src\sram_bank1.c
..\..\..\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -mz80:sms -o build\sms\sram_banks.sms obj\sms\main.o obj\sms\sram_bank0.o obj\sms\sram_bank1.o 

..\..\..\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -mz80:gg -c -o obj\gg\main.o src\main.c
..\..\..\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -mz80:gg -c -o obj\gg\sram_bank0.o src\sram_bank0.c
..\..\..\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -mz80:gg -c -o obj\gg\sram_bank1.o src\sram_bank1.c
..\..\..\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v  -mz80:gg -o build\gg\sram_banks.gg obj\gg\main.o obj\gg\sram_bank0.o obj\gg\sram_bank1.o 
