# Build the ROM:     make            (needs GBDK-2020, see README)
# Run rule tests:    make test       (needs only gcc)
# Level editor:      make editor     (needs gcc + python3; opens a browser)
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

.PHONY: all test test-editor tiles levels editor usage clean

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
		tests/test_game.c src/game.c src/world.c src/art.c src/trans.c src/levels.c tools/solver.c
	./build/test_game

# Editor bridge + "is src/levels.c up to date with levels/levels.json?"
test-editor: build/libgame.so
	python3 tests/test_editor.py

tiles:
	python3 tools/gen_tiles.py

# levels/levels.json is the source of truth for level data.
levels: src/levels.c

src/levels.c src/levels.h: levels/levels.json tools/gen_levels.py tools/tileset.py tools/png.py tilesets/*.json tilesets/*.png src/tiles.h src/trans.h
	python3 tools/gen_levels.py

# Level editor: game rules as a shared library + a local web server.
build/libgame.so: src/game.c src/game.h src/world.c src/world.h src/art.c src/art.h src/trans.c src/trans.h tools/solver.c tools/solver.h tools/editor_lib.c | build
	gcc -std=c99 -O2 -shared -fPIC -o $@ src/game.c src/world.c src/art.c src/trans.c tools/solver.c tools/editor_lib.c

editor: build/libgame.so
	python3 tools/editor/server.py

# How full the ROM banks and RAM are (GBDK's romusage tool).
usage: $(ROM)
	$(GBDK_HOME)/bin/romusage $(ROM) -g

clean:
	rm -rf build
