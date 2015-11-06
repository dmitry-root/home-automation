# Common build root for all firmware.

SPECIAL_DIRS := build output $(wildcard .*)
SUBDIRS := $(filter-out $(SPECIAL_DIRS),$(subst /,,$(wildcard */)))
BUILD_TYPE := $(if $(findstring debug,$(MAKECMDGOALS)),DEBUG,RELEASE)

export BUILD_TYPE

.PHONY: debug release all clean $(SUBDIRS)

debug:
release:

default: all

all: output $(SUBDIRS)

output:
	mkdir -p ../output/ti

clean:
	rm -rf ../output/ti

$(SUBDIRS):
	$(MAKE) -C $@ build
