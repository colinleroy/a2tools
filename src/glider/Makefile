SUBDIRS = \
	  bin/en \
	  bin/bg

all clean upload dist:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done
