# Build the ROM:     make            (needs GBDK-2020, see README)
# Run rule tests:    make test       (needs only gcc)
# Debug symbols:     make DEBUG=1    (for Emulicious source-level debugging)

GBDK_HOME ?= $(HOME)/gbdk
LCC       := $(GBDK_HOME)/bin/lcc

ROM  := build/stealth.gb
SRC  := $(wildcard src/*.c)
OBJ  := $(patsubst src/%.c,build/%.o,$(SRC))

CFLAGS  :=
# MBC5 cartridge (0x19), auto-size ROM banks, 15-char max title.
LDFLAGS := -Wm-yt0x19 -Wm-yoA -Wm-ynSTEALTH

ifeq ($(DEBUG),1)
CFLAGS  += -debug
LDFLAGS += -debug
endif

.PHONY: all test tiles clean

all: $(ROM)

$(ROM): $(OBJ)
	$(LCC) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c src/*.h | build
	$(LCC) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build

# Game rules compiled natively; includes a solver that proves every level
# can be finished.
test: | build
	gcc -std=c99 -Wall -Wextra -O2 -o build/test_game \
		tests/test_game.c src/game.c src/levels.c
	./build/test_game

tiles:
	python3 tools/gen_tiles.py

clean:
	rm -rf build
