# Makefile that generates needed rules for specified target
# Variables to be set:
#   SOURCES         -- (optional) list of board files to compile
#   board_OUTPUTS   -- (optional) list of output gerbers to generate
#   board_gerber_LAYERS -- (optional) list of layers to use for specified gerber file
# Possible output gerbers:
#   dimension, top, bottom, tstop, bstop, drill, ttext, btext

.PHONY: all

SOURCES ?= $(wildcard *.brd)
TARGETPATH := $(abspath $(CURDIR))
TARGETPATH := $(subst $(SRCROOT)/,,$(TARGETPATH))
TARGETPATH := $(strip $(TARGETPATH))
OUTDIR ?= $(OUTROOT)/$(TARGETPATH)

DEFAULT_OUTPUTS   := dimension top bottom tstop bstop drill ttext btext
SOURCE_BOARDS := $(SOURCES:%.brd=%)
list_gerbers = $(foreach gerber,$(or $($(board)_OUTPUTS),$(DEFAULT_OUTPUTS)),$(OUTDIR)/$(board)-$(gerber).gbr)
OUTPUT_FILES := $(foreach board,$(SOURCE_BOARDS),$(list_gerbers))

# default layers
DEFAULT_DIMENSION := Dimension
DEFAULT_TOP       := Top Pads Vias
DEFAULT_BOTTOM    := Bottom Pads Vias
DEFAULT_TSTOP     := tStop
DEFAULT_BSTOP     := bStop
DEFAULT_DRILL     := Drills Holes
DEFAULT_TTEXT     := tNames tPlace tKeepout tDocu
DEFAULT_BTEXT     := bNames bPlace bKeepout bDocu

all: $(OUTPUT_FILES)

$(OUTDIR)/%-dimension.gbr: %.brd Makefile
	@mkdir -p $(OUTDIR)
	$(EAGLE_CMD) -X -N -dGERBER_RS274X "-o$@" "$<" $(or $($(basename $<)_dimension_LAYERS),$(DEFAULT_DIMENSION))

$(OUTDIR)/%-top.gbr: %.brd Makefile
	@mkdir -p $(OUTDIR)
	$(EAGLE_CMD) -X -N -dGERBER_RS274X "-o$@" "$<" $(or $($(basename $<)_top_LAYERS),$(DEFAULT_TOP))

$(OUTDIR)/%-bottom.gbr: %.brd Makefile
	@mkdir -p $(OUTDIR)
	$(EAGLE_CMD) -X -N -dGERBER_RS274X "-o$@" "$<" $(or $($(basename $<)_bottom_LAYERS),$(DEFAULT_BOTTOM))

$(OUTDIR)/%-tstop.gbr: %.brd Makefile
	@mkdir -p $(OUTDIR)
	$(EAGLE_CMD) -X -N -dGERBER_RS274X "-o$@" "$<" $(or $($(basename $<)_tstop_LAYERS),$(DEFAULT_TSTOP))

$(OUTDIR)/%-bstop.gbr: %.brd Makefile
	@mkdir -p $(OUTDIR)
	$(EAGLE_CMD) -X -N -dGERBER_RS274X "-o$@" "$<" $(or $($(basename $<)_bstop_LAYERS),$(DEFAULT_BSTOP))

$(OUTDIR)/%-drill.gbr: %.brd Makefile
	@mkdir -p $(OUTDIR)
	$(EAGLE_CMD) -X -N -dEXCELLON "-o$@" "$<" $(or $($(basename $<)_drill_LAYERS),$(DEFAULT_DRILL))

$(OUTDIR)/%-ttext.gbr: %.brd Makefile
	@mkdir -p $(OUTDIR)
	$(EAGLE_CMD) -X -N -dGERBER_RS274X "-o$@" "$<" $(or $($(basename $<)_ttext_LAYERS),$(DEFAULT_TTEXT))

$(OUTDIR)/%-btext.gbr: %.brd Makefile
	@mkdir -p $(OUTDIR)
	$(EAGLE_CMD) -X -N -dGERBER_RS274X "-o$@" "$<" $(or $($(basename $<)_btext_LAYERS),$(DEFAULT_BTEXT))


