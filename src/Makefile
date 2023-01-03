SUBDIRS = lib a2recv mirror http-serial-proxy homecontrol-client

.PHONY: all clean

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@; \
	done
