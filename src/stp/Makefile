SUBDIRS = \
	  bin/6502 \
	  bin/65c02 \

all clean upload dist:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done
