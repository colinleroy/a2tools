SUBDIRS = a2trace \
	surl-server \
	glider \
	homecontrol-client \
	image-viewer \
	mastodon \
	quicktake \
	stp \
	telnet \
	weather \
	wozamp \
	woztubes \
	
INSTALL_SUBDIRS = \
	a2trace \
	surl-server \

.PHONY: all clean

all clean dist upload:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

#hack for debian packaging
install:
	for dir in $(INSTALL_SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

deb:
	debuild -S -sa --force-sign --no-check-builddeps

#deb-upload:
#	dput ppa:colin-colino/raspios surl-server_1.0_source.changes
