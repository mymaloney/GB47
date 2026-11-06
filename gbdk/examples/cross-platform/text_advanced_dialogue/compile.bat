REM Automatically generated from Makefile

..\..\..\\bin\png2asset  res\Font.png  -c gen\gb\src\Font.c -map -keep_palette_order  -noflip
..\..\..\\bin\png2asset  res\DialogueBox.png  -c gen\gb\src\DialogueBox.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc   -msm83:gb -Igen\gb\src -c -o obj\gb\main.o src\main.c
..\..\..\\bin\lcc   -msm83:gb -Igen\gb\src -o build\gb\text_advanced_dialogue.gb obj\gb\main.o   

..\..\..\\bin\png2asset  res\Font.png  -c gen\pocket\src\Font.c -map -keep_palette_order  -noflip
..\..\..\\bin\png2asset  res\DialogueBox.png  -c gen\pocket\src\DialogueBox.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc   -msm83:ap -Igen\pocket\src -c -o obj\pocket\main.o src\main.c
..\..\..\\bin\lcc   -msm83:ap -Igen\pocket\src -o build\pocket\text_advanced_dialogue.pocket obj\pocket\main.o   

..\..\..\\bin\png2asset  res\Font.png  -c gen\duck\src\Font.c -map -keep_palette_order  -noflip
..\..\..\\bin\png2asset  res\DialogueBox.png  -c gen\duck\src\DialogueBox.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc   -msm83:duck -Igen\duck\src -c -o obj\duck\main.o src\main.c
..\..\..\\bin\lcc   -msm83:duck -Igen\duck\src -o build\duck\text_advanced_dialogue.duck obj\duck\main.o   

..\..\..\\bin\png2asset  res\Font.png -pack_mode sms -bpp 4 -c gen\sms\src\Font.c -map -keep_palette_order  -noflip
..\..\..\\bin\png2asset  res\DialogueBox.png -pack_mode sms -bpp 4 -c gen\sms\src\DialogueBox.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc   -mz80:sms -Igen\sms\src -c -o obj\sms\main.o src\main.c
..\..\..\\bin\lcc   -mz80:sms -Igen\sms\src -o build\sms\text_advanced_dialogue.sms obj\sms\main.o   

..\..\..\\bin\png2asset  res\Font.png -pack_mode sms -bpp 4 -c gen\gg\src\Font.c -map -keep_palette_order  -noflip
..\..\..\\bin\png2asset  res\DialogueBox.png -pack_mode sms -bpp 4 -c gen\gg\src\DialogueBox.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc   -mz80:gg -Igen\gg\src -c -o obj\gg\main.o src\main.c
..\..\..\\bin\lcc   -mz80:gg -Igen\gg\src -o build\gg\text_advanced_dialogue.gg obj\gg\main.o   

..\..\..\\bin\png2asset  res\Font.png -pack_mode nes -bpp 2 -c gen\nes\src\Font.c -map -keep_palette_order  -noflip
..\..\..\\bin\png2asset  res\DialogueBox.png -pack_mode nes -bpp 2 -c gen\nes\src\DialogueBox.c -map -keep_palette_order -noflip
..\..\..\\bin\lcc   -mmos6502:nes -Igen\nes\src -c -o obj\nes\main.o src\main.c
..\..\..\\bin\lcc   -mmos6502:nes -Igen\nes\src -o build\nes\text_advanced_dialogue.nes obj\nes\main.o   
