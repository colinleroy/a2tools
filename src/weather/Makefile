SUBDIRS = \
	  bin/6502 \
	  bin/iigs

all clean upload dist:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done
