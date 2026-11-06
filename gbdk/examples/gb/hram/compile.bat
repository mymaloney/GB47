REM Automatically generated from Makefile
..\..\..\\bin\lcc -c -o hram_data.o hram_data.c
..\..\..\\bin\lcc -c -o hram.o hram.c
..\..\..\\bin\lcc -Wa-l -Wl-m -Wl-j -o hram.gb hram.o hram_data.o
