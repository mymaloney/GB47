# If you move this project you can change the directory
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
ifndef GBDK_HOME
	GBDK_HOME = ../../../
endif

LCC = $(GBDK_HOME)/bin/lcc
PNG2ASSET = $(GBDK_HOME)/bin/png2asset
GBCOMPRESS = $(GBDK_HOME)/bin/gbcompress

# Set platforms to build here, spaced separated. (These are in the separate Makefile.targets)
# They can also be built/cleaned individually: "make gg" and "make gg-clean"
# Possible are: gb gbc pocket megaduck sms gg
#TARGETS=gb pocket sms gg
TARGETS=gb gg sms nes

# Configure platform specific LCC flags here:
LCCFLAGS_gb      = -Wl-yt0x19 -Wl-yo4 -Wm-yn"$(PROJECTNAME)"
LCCFLAGS_pocket  = -Wl-yt0x19 -Wl-yo4 -Wm-yn"$(PROJECTNAME)"
LCCFLAGS_duck    = -Wl-yt0x19 -Wl-yo4 -Wm-yn"$(PROJECTNAME)"
LCCFLAGS_sms     = -Wl-yo4
LCCFLAGS_gg      = -Wl-yo4
LCCFLAGS_nes     = 

LCCFLAGS += $(LCCFLAGS_$(EXT)) # This adds the current platform specific LCC Flags

LCCFLAGS += -Wl-j -Wm-yoA -autobank -Wb-ext=.rel -Wb-v # MBC + Autobanking related flags

# GBDK_DEBUG = ON
ifdef GBDK_DEBUG
	LCCFLAGS += -debug -v
endif

CFLAGS = -Wf-Iinclude

# Add directory where platform specific meta tile sprites get converted into (obj/<platform ext>/src)
# So they can be included with "#include <res/somefile.h>"
CFLAGS += -I$(OBJDIR)  -I$(RESALLDIR)

# You can set the name of the ROM file here
PROJECTNAME = rledecompress

SRCDIR      = src
OBJDIR      = obj/$(EXT)
# In this example the converted output is the same on all platforms, and so shared for INCBIN() path simplicity
RESALLDIR   = obj/all
RESDIR      = res
BINDIR      = build/$(EXT)
MKDIRS      = $(OBJDIR) $(BINDIR) $(RESALLDIR) # See bottom of Makefile for directory auto-creation

BINS	    = $(OBJDIR)/$(PROJECTNAME).$(EXT)

# For .png -> png2asset -> .bin -> gbcompress -> .c -> compile -> .o
# When exporting to .bin the single png input file gets split into _map.bin and _tiles.bin, etc output files along with a header file
PNGS        = $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.png)))

# In this example, only the Map is compressed and not the Tiles
#
# So there is one list of uncompressed Tiles from pngs
PNG_RAW_TILEBINS = $(PNGS:%.png=$(RESALLDIR)/%_tiles.bin)  # tiles don't get compressed
#
# And another list of compressed Maps from the same pngs
PNG_RAW_MAPBINS  = $(PNGS:%.png=$(RESALLDIR)/%_map.bin)
PNG_COMP_MAPBINS = $(PNG_RAW_MAPBINS:$(RESALLDIR)%_map.bin=$(RESALLDIR)%_map.bin.rle)


CSOURCES    = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.c)))
ASMSOURCES  = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.s)))
OBJS        = $(CSOURCES:%.c=$(OBJDIR)/%.o) $(ASMSOURCES:%.s=$(OBJDIR)/%.o)

# Builds all targets sequentially
all: $(TARGETS)

compile.bat: Makefile
	@echo "REM Automatically generated from Makefile" > compile.bat
	@make -sn | sed y/\\//\\\\/ | grep -v make >> compile.bat


# ===== Start png conversion and compression =====

# For .png -> png2asset -> .bin -> gbcompress -> .bin.rle

# Use png2asset to convert the png into binary blob files for compression
# All of the multiple targets (map, tiles) will get generated
# by the single prerequisite of the matching %.png
$(RESALLDIR)/%_tiles.bin $(RESALLDIR)/%_map.bin:	$(RESDIR)/%.png
	$(PNG2ASSET) $< -o $(subst _map,,$(subst _tiles,,$@)) -bin -use_metafile

# Then compress the binary files and output as .c files
# Generate a variable name for the compressed C output based on stripping the path and extension from the output filename
$(RESALLDIR)/%.bin.rle:	$(RESALLDIR)/%.bin
	$(GBCOMPRESS) --alg=rle -v  $< $@

# ===== End png conversion and compression =====



# Compile .c files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.c
	$(LCC) $(LCCFLAGS) $(CFLAGS) -c -o $@ $<

# Compile .c files in "res/" to .o object files
$(OBJDIR)/%.o:	$(RESDIR)/%.c
	$(LCC) $(LCCFLAGS) $(CFLAGS) -c -o $@ $<

# Compile .s assembly files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.s
	$(LCC) $(LCCFLAGS) $(CFLAGS) -c -o $@ $<

# If needed, compile .c files in "src/" to .s assembly files
# (not required if .c is compiled directly to .o)
$(OBJDIR)/%.s:	$(SRCDIR)/%.c
	$(LCC) $(LCCFLAGS) $(CFLAGS) -S -o $@ $<


# Convert png images first so they're available when compiling the main sources
$(OBJS): $(PNG_RAW_TILEBINS) $(PNG_COMP_MAPBINS)

# Link the compiled object files into a .gb ROM file
$(BINS): $(OBJS)
	$(LCC) $(LCCFLAGS) $(CFLAGS) -o $(BINDIR)/$(PROJECTNAME).$(EXT) $(OBJS)

clean:
	@echo Cleaning
	@for target in $(TARGETS); do \
		$(MAKE) $$target-clean; \
	done

# Include available build targets
include Makefile.targets

# make -d to debug
# With builtin rules turned off for less noise:
# MAKEFLAGS += --no-builtin-rules


# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
ifneq ($(strip $(EXT)),)           # Only make the directories if EXT has been set by a target
$(info $(shell mkdir -p $(MKDIRS)))
endif
