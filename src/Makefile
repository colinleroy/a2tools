SUBDIRS = lib a2trace surl-server \
	homecontrol-client mastodon telnet surl stp quicktake image-viewer \
	wozamp iinvidious
INSTALL_SUBDIRS = a2trace surl-server
UPLOAD_SUBDIRS = homecontrol-client mastodon telnet surl stp quicktake image-viewer wozamp

.PHONY: all clean

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

#hack for debian packaging
install:
	for dir in $(INSTALL_SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

upload:
	for dir in $(UPLOAD_SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

deb:
	debuild -S -sa --force-sign --no-check-builddeps

#deb-upload:
#	dput ppa:colin-colino/raspios surl-server_1.0_source.changes
