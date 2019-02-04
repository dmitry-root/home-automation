# Build file for all board files

# == Directories ==
export BUILDROOT := $(CURDIR)/build
export COMMONROOT := $(abspath $(BUILDROOT))
export SRCROOT := $(abspath $(CURDIR))
export OUTROOT := $(abspath $(CURDIR)/gerber)
export ZIPROOT := $(abspath $(CURDIR)/zip)

export MAKE_BUILD := $(BUILDROOT)/build.mk
export MAKE_SUBDIRS := $(BUILDROOT)/subdirs.mk

export SH ?= /bin/sh
export EAGLE_CMD ?= eagle

.PHONY: all zip clean

all: $(OUTROOT)
	@$(MAKE) -C sensor all
	@$(MAKE) -C ti_can all
	@$(MAKE) -C pi_cal all

zip: $(ZIPROOT)
	/usr/bin/perl $(BUILDROOT)/pack_gerbers.pl gerber zip

clean:
	rm -rf $(OUTROOT)/* $(ZIPROOT)/*

$(OUTROOT):
	mkdir -p $(OUTROOT)

$(ZIPROOT):
	mkdir -p $(ZIPROOT)

