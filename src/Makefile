SUBDIRS = lib a2recv mirror surl-server homecontrol-client surl stp aoc

.PHONY: all clean

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done
