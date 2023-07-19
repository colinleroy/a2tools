SUBDIRS = lib a2send a2recv surl-server homecontrol-client mastodon telnet surl stp

.PHONY: all clean

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

#hack for debian packaging
install:
	$(MAKE) -C surl-server -f Makefile $@ || exit

deb:
	debuild -S -sa --force-sign

#deb-upload:
#	dput ppa:colin-colino/raspios surl-server_1.0_source.changes
