# Common build root for all firmware.

SPECIAL_DIRS := build include $(wildcard .*)
SUBDIRS := $(filter-out $(SPECIAL_DIRS),$(subst /,,$(wildcard */)))
BUILD_TYPE := $(if $(findstring debug,$(MAKECMDGOALS)),DEBUG,RELEASE)

export BUILD_TYPE

.PHONY: debug release all clean $(SUBDIRS)

default: all

debug:
release:

all: output $(SUBDIRS)

output:
	mkdir -p ../../output/firmware/ti

clean:
	rm -rf ../../output/firmware/ti

$(SUBDIRS):
	$(MAKE) -C $@ build

heater: onewire
