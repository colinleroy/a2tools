SUBDIRS = lib a2send a2recv surl-server homecontrol-client mastodon telnet surl stp

.PHONY: all clean

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done
