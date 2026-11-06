SHELL := /bin/bash

# If you move this project you can change the directory
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
ifndef GBDK_HOME
	GBDK_HOME = ../../../
endif

LCC	= $(GBDK_HOME)/bin/lcc

BINS	= hram.gb

all:	$(BINS)

compile.bat:	Makefile
	@echo "REM Automatically generated from Makefile" > compile.bat
	@make -sn | sed y/\\//\\\\/ | sed s/mkdir\ -p\/mkdir\/ | grep -v make >> compile.bat

$(BINS):
	$(LCC) -c -o hram_data.o hram_data.c
	$(LCC) -c -o hram.o hram.c
	$(LCC) -Wa-l -Wl-m -Wl-j -o $(BINS) hram.o hram_data.o

clean:
	rm -f *.o *.lst *.map *.gb *~ *.rel *.cdb *.ihx *.lnk *.sym *.asm *.noi *.rst

