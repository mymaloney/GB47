REM Automatically generated from Makefile

..\..\..\\bin\png2asset res\bigmap.png -map -bpp 2 -max_palettes 4 -pack_mode gb -noflip -c obj\gb\res\bigmap.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gb  -msm83:gb -c -o obj\gb\bigmap.o obj\gb\res\bigmap.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gb  -msm83:gb -c -o obj\gb\large_map.o src\large_map.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gb  -msm83:gb -o build\gb\large-map.gb obj\gb\bigmap.o obj\gb\large_map.o 

..\..\..\\bin\png2asset res\bigmap.png -map -use_map_attributes -bpp 2 -max_palettes 8 -pack_mode gb -c obj\gbc\res\bigmap.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gbc  -msm83:gb -c -o obj\gbc\bigmap.o obj\gbc\res\bigmap.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gbc  -msm83:gb -c -o obj\gbc\large_map.o src\large_map.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gbc  -msm83:gb -o build\gbc\large-map.gbc obj\gbc\bigmap.o obj\gbc\large_map.o 

..\..\..\\bin\png2asset res\bigmap.png -map -use_map_attributes -bpp 4 -max_palettes 2 -pack_mode sms -c obj\gg\res\bigmap.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gg  -mz80:gg -c -o obj\gg\bigmap.o obj\gg\res\bigmap.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gg  -mz80:gg -c -o obj\gg\large_map.o src\large_map.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gg  -mz80:gg -o build\gg\large-map.gg obj\gg\bigmap.o obj\gg\large_map.o 

..\..\..\\bin\png2asset res\bigmap.png -map -use_map_attributes -bpp 4 -max_palettes 2 -pack_mode sms -c obj\sms\res\bigmap.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\sms  -mz80:sms -c -o obj\sms\bigmap.o obj\sms\res\bigmap.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\sms  -mz80:sms -c -o obj\sms\large_map.o src\large_map.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\sms  -mz80:sms -o build\sms\large-map.sms obj\sms\bigmap.o obj\sms\large_map.o 

..\..\..\\bin\png2asset res\bigmap.png -map -bpp 2 -max_palettes 4 -pack_mode nes -noflip -use_nes_attributes -b 0 -c obj\nes\res\bigmap.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\nes  -mmos6502:nes -c -o obj\nes\bigmap.o obj\nes\res\bigmap.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\nes  -mmos6502:nes -c -o obj\nes\large_map.o src\large_map.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\nes  -mmos6502:nes -o build\nes\large-map.nes obj\nes\bigmap.o obj\nes\large_map.o 
