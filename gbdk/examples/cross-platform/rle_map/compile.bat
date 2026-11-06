REM Automatically generated from Makefile

..\..\..\\bin\png2asset res\map.png -o obj\all\map.bin -bin -use_metafile
..\..\..\\bin\gbcompress --alg=rle -v  obj\all\map_map.bin obj\all\map_map.bin.rle
..\..\..\\bin\lcc -Wl-yt0x19 -Wl-yo4 -Wm-yn"rledecompress"  -Wl-j -Wm-yoA -autobank -Wb-ext=.rel -Wb-v  -Wf-Iinclude -Iobj\gb  -Iobj\all -msm83:gb -c -o obj\gb\main.o src\main.c
..\..\..\\bin\lcc -Wl-yt0x19 -Wl-yo4 -Wm-yn"rledecompress"  -Wl-j -Wm-yoA -autobank -Wb-ext=.rel -Wb-v  -Wf-Iinclude -Iobj\gb  -Iobj\all -msm83:gb -o build\gb\rledecompress.gb obj\gb\main.o 

..\..\..\\bin\png2asset res\map.png -o obj\all\map.bin -bin -use_metafile
..\..\..\\bin\gbcompress --alg=rle -v  obj\all\map_map.bin obj\all\map_map.bin.rle
..\..\..\\bin\lcc -Wl-yo4  -Wl-j -Wm-yoA -autobank -Wb-ext=.rel -Wb-v  -Wf-Iinclude -Iobj\gg  -Iobj\all -mz80:gg -c -o obj\gg\main.o src\main.c
..\..\..\\bin\lcc -Wl-yo4  -Wl-j -Wm-yoA -autobank -Wb-ext=.rel -Wb-v  -Wf-Iinclude -Iobj\gg  -Iobj\all -mz80:gg -o build\gg\rledecompress.gg obj\gg\main.o 

..\..\..\\bin\png2asset res\map.png -o obj\all\map.bin -bin -use_metafile
..\..\..\\bin\gbcompress --alg=rle -v  obj\all\map_map.bin obj\all\map_map.bin.rle
..\..\..\\bin\lcc -Wl-yo4  -Wl-j -Wm-yoA -autobank -Wb-ext=.rel -Wb-v  -Wf-Iinclude -Iobj\sms  -Iobj\all -mz80:sms -c -o obj\sms\main.o src\main.c
..\..\..\\bin\lcc -Wl-yo4  -Wl-j -Wm-yoA -autobank -Wb-ext=.rel -Wb-v  -Wf-Iinclude -Iobj\sms  -Iobj\all -mz80:sms -o build\sms\rledecompress.sms obj\sms\main.o 

..\..\..\\bin\png2asset res\map.png -o obj\all\map.bin -bin -use_metafile
..\..\..\\bin\gbcompress --alg=rle -v  obj\all\map_map.bin obj\all\map_map.bin.rle
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -autobank -Wb-ext=.rel -Wb-v  -Wf-Iinclude -Iobj\nes  -Iobj\all -mmos6502:nes -c -o obj\nes\main.o src\main.c
..\..\..\\bin\lcc   -Wl-j -Wm-yoA -autobank -Wb-ext=.rel -Wb-v  -Wf-Iinclude -Iobj\nes  -Iobj\all -mmos6502:nes -o build\nes\rledecompress.nes obj\nes\main.o 
