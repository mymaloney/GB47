REM Automatically generated from Makefile

..\..\..\\bin\png2asset  res\Font.png  -c gen\gb\src\Font.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc -Wm-ys -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -msm83:gb -Igen\gb\src -c -o obj\gb\main.o src\main.c
..\..\..\\bin\lcc -Wm-ys -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -msm83:gb -Igen\gb\src -o build\gb\text_typewriter.gb obj\gb\main.o   

..\..\..\\bin\png2asset  res\Font.png  -c gen\pocket\src\Font.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc -Wm-ys -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -msm83:ap -Igen\pocket\src -c -o obj\pocket\main.o src\main.c
..\..\..\\bin\lcc -Wm-ys -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -msm83:ap -Igen\pocket\src -o build\pocket\text_typewriter.pocket obj\pocket\main.o   

..\..\..\\bin\png2asset  res\Font.png  -c gen\duck\src\Font.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc -Wm-ys -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -msm83:duck -Igen\duck\src -c -o obj\duck\main.o src\main.c
..\..\..\\bin\lcc -Wm-ys -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -msm83:duck -Igen\duck\src -o build\duck\text_typewriter.duck obj\duck\main.o   

..\..\..\\bin\png2asset  res\Font.png -pack_mode sms -bpp 4 -c gen\sms\src\Font.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -mz80:sms -Igen\sms\src -c -o obj\sms\main.o src\main.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -mz80:sms -Igen\sms\src -o build\sms\text_typewriter.sms obj\sms\main.o   

..\..\..\\bin\png2asset  res\Font.png -pack_mode sms -bpp 4 -c gen\gg\src\Font.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -mz80:gg -Igen\gg\src -c -o obj\gg\main.o src\main.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -mz80:gg -Igen\gg\src -o build\gg\text_typewriter.gg obj\gg\main.o   

..\..\..\\bin\png2asset  res\Font.png -pack_mode nes -bpp 2 -c gen\nes\src\Font.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -mmos6502:nes -Igen\nes\src -c -o obj\nes\main.o src\main.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -mmos6502:nes -Igen\nes\src -o build\nes\text_typewriter.nes obj\nes\main.o   
