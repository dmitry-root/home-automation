# Generate needed targets by propagating compilation of $(SUBDIRS) directories

.PHONY: all install $(SUBDIRS)

all: TGT = all
all: $(SUBDIRS)

install: TGT = install
install: $(SUBDIRS)

$(SUBDIRS): %:
	@$(MAKE) -C $@ $(TGT)

