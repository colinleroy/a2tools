SUBDIRS = day9 day10 day11 day12 day13 day14 day15 day16 day17 day18 day19 day20 day21 day22 day23

.PHONY: all clean

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@; \
	done
