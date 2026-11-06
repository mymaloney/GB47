REM Automatically generated from Makefile

..\..\..\\bin\png2asset res\scene00001.png -map -bpp 2 -max_palettes 4 -pack_mode gb -noflip -c obj\gb\res\scene00001.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gb  -msm83:gb -c -o obj\gb\scene00001.o obj\gb\res\scene00001.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gb  -msm83:gb -c -o obj\gb\gbprinter.o src\gbprinter.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gb  -msm83:gb -c -o obj\gb\print_example.o src\print_example.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gb  -msm83:gb -o build\gb\GBPrinter.gb obj\gb\scene00001.o obj\gb\gbprinter.o obj\gb\print_example.o 

..\..\..\\bin\png2asset res\scene00001.png -map -bpp 2 -max_palettes 4 -pack_mode gb -noflip -c obj\gbc\res\scene00001.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gbc  -msm83:gb -c -o obj\gbc\scene00001.o obj\gbc\res\scene00001.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gbc  -msm83:gb -c -o obj\gbc\gbprinter.o src\gbprinter.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gbc  -msm83:gb -c -o obj\gbc\print_example.o src\print_example.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gbc  -msm83:gb -o build\gbc\GBPrinter.gbc obj\gbc\scene00001.o obj\gbc\gbprinter.o obj\gbc\print_example.o 

..\..\..\\bin\png2asset res\scene00001.png -map -bpp 2 -max_palettes 4 -pack_mode gb -noflip -c obj\duck\res\scene00001.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\duck  -msm83:duck -c -o obj\duck\scene00001.o obj\duck\res\scene00001.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\duck  -msm83:duck -c -o obj\duck\gbprinter.o src\gbprinter.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\duck  -msm83:duck -c -o obj\duck\print_example.o src\print_example.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\duck  -msm83:duck -o build\duck\GBPrinter.duck obj\duck\scene00001.o obj\duck\gbprinter.o obj\duck\print_example.o 

..\..\..\\bin\png2asset res\scene00001.png -map -bpp 2 -max_palettes 4 -pack_mode gb -noflip -c obj\gg\res\scene00001.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gg  -mz80:gg -c -o obj\gg\scene00001.o obj\gg\res\scene00001.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gg  -mz80:gg -c -o obj\gg\gbprinter.o src\gbprinter.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gg  -mz80:gg -c -o obj\gg\print_example.o src\print_example.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wf-MMD -Wf-Wp-MP  -Iobj\gg  -mz80:gg -o build\gg\GBPrinter.gg obj\gg\scene00001.o obj\gg\gbprinter.o obj\gg\print_example.o 
