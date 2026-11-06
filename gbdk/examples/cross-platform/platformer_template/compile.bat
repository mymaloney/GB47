REM Automatically generated from Makefile

..\..\..\\bin\png2asset  res\graphics\player-character-gbapduck-sprites.png -c gen\gb\src\PlayerCharacterSprites.c -px 12 -py 6 -spr8x16 -keep_palette_order -sw 24 -sh 32  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-tileset.png -c gen\gb\src\World1Tileset.c -keep_palette_order -noflip -map  -b 255
..\..\..\\bin\png2asset  res\graphics\world2-tileset.png -c gen\gb\src\World2Tileset.c -keep_palette_order -noflip -map  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area1.png -c gen\gb\src\World1Area1.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png   -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area2.png -c gen\gb\src\World1Area2.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png  -b 255
..\..\..\\bin\png2asset  res\graphics\world2-area1.png -c gen\gb\src\World2Area1.c -noflip -map -maps_only -source_tileset res\graphics\world2-tileset.png  -b 255
..\..\..\\bin\png2asset  res\graphics\title-screen.png -c gen\gb\src\TitleScreen.c -noflip -map   -b 255
..\..\..\\bin\png2asset  res\graphics\next-level.png -c gen\gb\src\NextLevel.c -noflip -map  -b 255
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:gb -Igen\gb\src -Ires -c -o obj\gb\camera.o src\camera.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:gb -Igen\gb\src -Ires -c -o obj\gb\common.o src\common.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:gb -Igen\gb\src -Ires -c -o obj\gb\level.o src\level.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:gb -Igen\gb\src -Ires -c -o obj\gb\main.o src\main.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:gb -Igen\gb\src -Ires -c -o obj\gb\player.o src\player.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:gb -Igen\gb\src -o build\gb\platformer_template.gb obj\gb\camera.o obj\gb\common.o obj\gb\level.o obj\gb\main.o obj\gb\player.o   

..\..\..\\bin\png2asset  res\graphics\player-character-gbapduck-sprites.png -c gen\pocket\src\PlayerCharacterSprites.c -px 12 -py 6 -spr8x16 -keep_palette_order -sw 24 -sh 32  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-tileset.png -c gen\pocket\src\World1Tileset.c -keep_palette_order -noflip -map  -b 255
..\..\..\\bin\png2asset  res\graphics\world2-tileset.png -c gen\pocket\src\World2Tileset.c -keep_palette_order -noflip -map  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area1.png -c gen\pocket\src\World1Area1.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png   -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area2.png -c gen\pocket\src\World1Area2.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png  -b 255
..\..\..\\bin\png2asset  res\graphics\world2-area1.png -c gen\pocket\src\World2Area1.c -noflip -map -maps_only -source_tileset res\graphics\world2-tileset.png  -b 255
..\..\..\\bin\png2asset  res\graphics\title-screen.png -c gen\pocket\src\TitleScreen.c -noflip -map   -b 255
..\..\..\\bin\png2asset  res\graphics\next-level.png -c gen\pocket\src\NextLevel.c -noflip -map  -b 255
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:ap -Igen\pocket\src -Ires -c -o obj\pocket\camera.o src\camera.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:ap -Igen\pocket\src -Ires -c -o obj\pocket\common.o src\common.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:ap -Igen\pocket\src -Ires -c -o obj\pocket\level.o src\level.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:ap -Igen\pocket\src -Ires -c -o obj\pocket\main.o src\main.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:ap -Igen\pocket\src -Ires -c -o obj\pocket\player.o src\player.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:ap -Igen\pocket\src -o build\pocket\platformer_template.pocket obj\pocket\camera.o obj\pocket\common.o obj\pocket\level.o obj\pocket\main.o obj\pocket\player.o   

..\..\..\\bin\png2asset  res\graphics\player-character-gbapduck-sprites.png -c gen\duck\src\PlayerCharacterSprites.c -px 12 -py 6 -spr8x16 -keep_palette_order -sw 24 -sh 32  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-tileset.png -c gen\duck\src\World1Tileset.c -keep_palette_order -noflip -map  -b 255
..\..\..\\bin\png2asset  res\graphics\world2-tileset.png -c gen\duck\src\World2Tileset.c -keep_palette_order -noflip -map  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area1.png -c gen\duck\src\World1Area1.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png   -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area2.png -c gen\duck\src\World1Area2.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png  -b 255
..\..\..\\bin\png2asset  res\graphics\world2-area1.png -c gen\duck\src\World2Area1.c -noflip -map -maps_only -source_tileset res\graphics\world2-tileset.png  -b 255
..\..\..\\bin\png2asset  res\graphics\title-screen.png -c gen\duck\src\TitleScreen.c -noflip -map   -b 255
..\..\..\\bin\png2asset  res\graphics\next-level.png -c gen\duck\src\NextLevel.c -noflip -map  -b 255
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:duck -Igen\duck\src -Ires -c -o obj\duck\camera.o src\camera.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:duck -Igen\duck\src -Ires -c -o obj\duck\common.o src\common.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:duck -Igen\duck\src -Ires -c -o obj\duck\level.o src\level.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:duck -Igen\duck\src -Ires -c -o obj\duck\main.o src\main.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:duck -Igen\duck\src -Ires -c -o obj\duck\player.o src\player.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank   -msm83:duck -Igen\duck\src -o build\duck\platformer_template.duck obj\duck\camera.o obj\duck\common.o obj\duck\level.o obj\duck\main.o obj\duck\player.o   

..\..\..\\bin\png2asset  res\graphics\player-character-ggsms-sprites.png -c gen\sms\src\PlayerCharacterSprites.c -px 12 -py 6 -spr8x16 -keep_palette_order -sw 24 -sh 32 -noflip -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\world1-tileset.png -c gen\sms\src\World1Tileset.c -keep_palette_order -noflip -map -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\world2-tileset.png -c gen\sms\src\World2Tileset.c -keep_palette_order -noflip -map -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area1.png -c gen\sms\src\World1Area1.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png -pack_mode sms -bpp 4  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area2.png -c gen\sms\src\World1Area2.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\world2-area1.png -c gen\sms\src\World2Area1.c -noflip -map -maps_only -source_tileset res\graphics\world2-tileset.png -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\title-screen.png -c gen\sms\src\TitleScreen.c -noflip -map -pack_mode sms -bpp 4  -b 255
..\..\..\\bin\png2asset  res\graphics\next-level.png -c gen\sms\src\NextLevel.c -noflip -map -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:sms -Igen\sms\src -Ires -c -o obj\sms\camera.o src\camera.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:sms -Igen\sms\src -Ires -c -o obj\sms\common.o src\common.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:sms -Igen\sms\src -Ires -c -o obj\sms\level.o src\level.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:sms -Igen\sms\src -Ires -c -o obj\sms\main.o src\main.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:sms -Igen\sms\src -Ires -c -o obj\sms\player.o src\player.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:sms -Igen\sms\src -o build\sms\platformer_template.sms obj\sms\camera.o obj\sms\common.o obj\sms\level.o obj\sms\main.o obj\sms\player.o   

..\..\..\\bin\png2asset  res\graphics\player-character-ggsms-sprites.png -c gen\gg\src\PlayerCharacterSprites.c -px 12 -py 6 -spr8x16 -keep_palette_order -sw 24 -sh 32 -noflip -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\world1-tileset.png -c gen\gg\src\World1Tileset.c -keep_palette_order -noflip -map -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\world2-tileset.png -c gen\gg\src\World2Tileset.c -keep_palette_order -noflip -map -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area1.png -c gen\gg\src\World1Area1.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png -pack_mode sms -bpp 4  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area2.png -c gen\gg\src\World1Area2.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\world2-area1.png -c gen\gg\src\World2Area1.c -noflip -map -maps_only -source_tileset res\graphics\world2-tileset.png -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\png2asset  res\graphics\title-screen.png -c gen\gg\src\TitleScreen.c -noflip -map -pack_mode sms -bpp 4  -b 255
..\..\..\\bin\png2asset  res\graphics\next-level.png -c gen\gg\src\NextLevel.c -noflip -map -pack_mode sms -bpp 4 -b 255
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:gg -Igen\gg\src -Ires -c -o obj\gg\camera.o src\camera.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:gg -Igen\gg\src -Ires -c -o obj\gg\common.o src\common.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:gg -Igen\gg\src -Ires -c -o obj\gg\level.o src\level.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:gg -Igen\gg\src -Ires -c -o obj\gg\main.o src\main.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:gg -Igen\gg\src -Ires -c -o obj\gg\player.o src\player.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mz80:gg -Igen\gg\src -o build\gg\platformer_template.gg obj\gg\camera.o obj\gg\common.o obj\gg\level.o obj\gg\main.o obj\gg\player.o   

..\..\..\\bin\png2asset  res\graphics\player-character-nes-sprites.png -c gen\nes\src\PlayerCharacterSprites.c -px 12 -py 6 -spr8x16 -keep_palette_order -sw 24 -sh 32  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-tileset.png -c gen\nes\src\World1Tileset.c -keep_palette_order -noflip -map -noflip -bpp 2 -pack_mode nes -b 255
..\..\..\\bin\png2asset  res\graphics\world2-tileset.png -c gen\nes\src\World2Tileset.c -keep_palette_order -noflip -map -noflip -bpp 2 -pack_mode nes -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area1.png -c gen\nes\src\World1Area1.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png -noflip -bpp 2 -pack_mode nes  -b 255
..\..\..\\bin\png2asset  res\graphics\world1-area2.png -c gen\nes\src\World1Area2.c -noflip -map -maps_only -source_tileset res\graphics\world1-tileset.png -noflip -bpp 2 -pack_mode nes -b 255
..\..\..\\bin\png2asset  res\graphics\world2-area1.png -c gen\nes\src\World2Area1.c -noflip -map -maps_only -source_tileset res\graphics\world2-tileset.png -noflip -bpp 2 -pack_mode nes -b 255
..\..\..\\bin\png2asset  res\graphics\title-screen.png -c gen\nes\src\TitleScreen.c -noflip -map -noflip -bpp 2 -pack_mode nes  -b 255
..\..\..\\bin\png2asset  res\graphics\next-level.png -c gen\nes\src\NextLevel.c -noflip -map -noflip -bpp 2 -pack_mode nes -b 255
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mmos6502:nes -Igen\nes\src -Ires -c -o obj\nes\camera.o src\camera.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mmos6502:nes -Igen\nes\src -Ires -c -o obj\nes\common.o src\common.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mmos6502:nes -Igen\nes\src -Ires -c -o obj\nes\level.o src\level.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mmos6502:nes -Igen\nes\src -Ires -c -o obj\nes\main.o src\main.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mmos6502:nes -Igen\nes\src -Ires -c -o obj\nes\player.o src\player.c
..\..\..\\bin\lcc  -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wm-ys -Wl-yt0x1B -autobank  -mmos6502:nes -Igen\nes\src -o build\nes\platformer_template.nes obj\nes\camera.o obj\nes\common.o obj\nes\level.o obj\nes\main.o obj\nes\player.o   
