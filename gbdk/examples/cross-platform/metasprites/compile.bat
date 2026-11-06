REM Automatically generated from Makefile

..\..\..\\bin\png2asset res\sprite.png -sh 48 -spr8x8 -noflip -c obj\gb\res\sprite.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\gb -msm83:gb -c -o obj\gb\sprite.o obj\gb\res\sprite.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\gb -msm83:gb -c -o obj\gb\metasprites.o src\metasprites.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\gb -msm83:gb -o build\gb\metasprites.gb obj\gb\sprite.o obj\gb\metasprites.o 

..\..\..\\bin\png2asset res\sprite.png -sh 48 -spr8x8 -noflip -c obj\gbc\res\sprite.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\gbc -msm83:gb -c -o obj\gbc\sprite.o obj\gbc\res\sprite.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\gbc -msm83:gb -c -o obj\gbc\metasprites.o src\metasprites.c
..\..\..\\bin\lcc -Wl-yt0x1B -Wm-yc -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\gbc -msm83:gb -o build\gbc\metasprites.gbc obj\gbc\sprite.o obj\gbc\metasprites.o 

..\..\..\\bin\png2asset res\sprite.png -sh 48 -spr8x8 -noflip -c obj\pocket\res\sprite.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\pocket -msm83:ap -c -o obj\pocket\sprite.o obj\pocket\res\sprite.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\pocket -msm83:ap -c -o obj\pocket\metasprites.o src\metasprites.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\pocket -msm83:ap -o build\pocket\metasprites.pocket obj\pocket\sprite.o obj\pocket\metasprites.o 

..\..\..\\bin\png2asset res\sprite.png -sh 48 -spr8x8 -noflip -c obj\duck\res\sprite.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\duck -msm83:duck -c -o obj\duck\sprite.o obj\duck\res\sprite.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\duck -msm83:duck -c -o obj\duck\metasprites.o src\metasprites.c
..\..\..\\bin\lcc -Wl-yt0x1B -autobank   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\duck -msm83:duck -o build\duck\metasprites.duck obj\duck\sprite.o obj\duck\metasprites.o 

..\..\..\\bin\png2asset res\sprite.png -sh 48 -spr8x8 -noflip -c obj\sms\res\sprite.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\sms -mz80:sms -c -o obj\sms\sprite.o obj\sms\res\sprite.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\sms -mz80:sms -c -o obj\sms\metasprites.o src\metasprites.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\sms -mz80:sms -o build\sms\metasprites.sms obj\sms\sprite.o obj\sms\metasprites.o 

..\..\..\\bin\png2asset res\sprite.png -sh 48 -spr8x8 -noflip -c obj\gg\res\sprite.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\gg -mz80:gg -c -o obj\gg\sprite.o obj\gg\res\sprite.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\gg -mz80:gg -c -o obj\gg\metasprites.o src\metasprites.c
..\..\..\\bin\lcc -autobank  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\gg -mz80:gg -o build\gg\metasprites.gg obj\gg\sprite.o obj\gg\metasprites.o 

..\..\..\\bin\png2asset res\sprite.png -sh 48 -spr8x8 -noflip -c obj\nes\res\sprite.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\nes -mmos6502:nes -c -o obj\nes\sprite.o obj\nes\res\sprite.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\nes -mmos6502:nes -c -o obj\nes\metasprites.o src\metasprites.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v  -Iobj\nes -mmos6502:nes -o build\nes\metasprites.nes obj\nes\sprite.o obj\nes\metasprites.o 
