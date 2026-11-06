REM Automatically generated from Makefile

..\..\..\\bin\png2asset res\DMG\GBDK_2020_logo.png `cat <res\DMG\GBDK_2020_logo.png.meta 2>\dev\null` -c obj\gb\GBDK_2020_logo.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\gb -DSYSTEM_DMG -msm83:gb -c -o obj\gb\GBDK_2020_logo.o obj\gb\GBDK_2020_logo.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\gb -DSYSTEM_DMG -msm83:gb -c -o obj\gb\main.o src\main.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\gb -DSYSTEM_DMG -msm83:gb -o build\gb\logo.gb obj\gb\GBDK_2020_logo.o obj\gb\main.o 

..\..\..\\bin\png2asset res\CGB\GBDK_2020_logo.png `cat <res\CGB\GBDK_2020_logo.png.meta 2>\dev\null` -c obj\gbc\GBDK_2020_logo.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\gbc -DSYSTEM_CGB -msm83:gb -c -o obj\gbc\GBDK_2020_logo.o obj\gbc\GBDK_2020_logo.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\gbc -DSYSTEM_CGB -msm83:gb -c -o obj\gbc\main.o src\main.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\gbc -DSYSTEM_CGB -msm83:gb -o build\gbc\logo.gbc obj\gbc\GBDK_2020_logo.o obj\gbc\main.o 

..\..\..\\bin\png2asset res\DMG\GBDK_2020_logo.png `cat <res\DMG\GBDK_2020_logo.png.meta 2>\dev\null` -c obj\pocket\GBDK_2020_logo.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\pocket -DSYSTEM_DMG -msm83:ap -c -o obj\pocket\GBDK_2020_logo.o obj\pocket\GBDK_2020_logo.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\pocket -DSYSTEM_DMG -msm83:ap -c -o obj\pocket\main.o src\main.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\pocket -DSYSTEM_DMG -msm83:ap -o build\pocket\logo.pocket obj\pocket\GBDK_2020_logo.o obj\pocket\main.o 

..\..\..\\bin\png2asset res\DMG\GBDK_2020_logo.png `cat <res\DMG\GBDK_2020_logo.png.meta 2>\dev\null` -c obj\duck\GBDK_2020_logo.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\duck -DSYSTEM_DMG -msm83:duck -c -o obj\duck\GBDK_2020_logo.o obj\duck\GBDK_2020_logo.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\duck -DSYSTEM_DMG -msm83:duck -c -o obj\duck\main.o src\main.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\duck -DSYSTEM_DMG -msm83:duck -o build\duck\logo.duck obj\duck\GBDK_2020_logo.o obj\duck\main.o 

..\..\..\\bin\png2asset res\SEGA\GBDK_2020_logo.png `cat <res\SEGA\GBDK_2020_logo.png.meta 2>\dev\null` -c obj\sms\GBDK_2020_logo.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\sms -DSYSTEM_SEGA -mz80:sms -c -o obj\sms\GBDK_2020_logo.o obj\sms\GBDK_2020_logo.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\sms -DSYSTEM_SEGA -mz80:sms -c -o obj\sms\main.o src\main.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\sms -DSYSTEM_SEGA -mz80:sms -o build\sms\logo.sms obj\sms\GBDK_2020_logo.o obj\sms\main.o 

..\..\..\\bin\png2asset res\SEGA\GBDK_2020_logo.png `cat <res\SEGA\GBDK_2020_logo.png.meta 2>\dev\null` -c obj\gg\GBDK_2020_logo.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\gg -DSYSTEM_SEGA -mz80:gg -c -o obj\gg\GBDK_2020_logo.o obj\gg\GBDK_2020_logo.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\gg -DSYSTEM_SEGA -mz80:gg -c -o obj\gg\main.o src\main.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\gg -DSYSTEM_SEGA -mz80:gg -o build\gg\logo.gg obj\gg\GBDK_2020_logo.o obj\gg\main.o 

..\..\..\\bin\png2asset res\NES\GBDK_2020_logo.png `cat <res\NES\GBDK_2020_logo.png.meta 2>\dev\null` -c obj\nes\GBDK_2020_logo.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\nes -DSYSTEM_NES -mmos6502:nes -c -o obj\nes\GBDK_2020_logo.o obj\nes\GBDK_2020_logo.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\nes -DSYSTEM_NES -mmos6502:nes -c -o obj\nes\main.o src\main.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Wm-yn"LOGO "  -Iobj\nes -DSYSTEM_NES -mmos6502:nes -o build\nes\logo.nes obj\nes\GBDK_2020_logo.o obj\nes\main.o 
