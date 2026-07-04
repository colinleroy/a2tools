SUBDIRS = a2trace \
	surl-server \
	glider \
	shufflepuck \
	ammonoid \
	homecontrol-client \
	image-viewer \
	mastodon \
	quicktake \
	sixforty \
	stp \
	telnet \
	weather \
	wozamp \
	woztubes \
	
INSTALL_SUBDIRS = \
	a2trace \
	surl-server \

HD_IMG = ../dist/colin-s-things.hdv
HD_SUBDIRS = \
	glider/bin/en \
	shufflepuck \
	ammonoid \
	image-viewer \
	mastodon \
	quicktake \
	sixforty \
	stp \
	telnet \
	weather \
	wozamp \
	woztubes \

.PHONY: all clean

all clean dist upload:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

hdv:
	rm -f $(HD_IMG)
	../bin/cp2/cp2 create-disk-image $(HD_IMG) 32M prodos 
	../bin/cp2/cp2 rename $(HD_IMG) : COLIN.S.THINGS
	for file in BASIC.SYSTEM BITSY.BOOT QUIT.SYSTEM PRODOS; do \
		../bin/cp2/cp2 add --strip-paths $(HD_IMG) ../bin/$$file ; \
	done
	for dir in $(HD_SUBDIRS); do \
		IMG=../$(HD_IMG) $(MAKE) -C $$dir -f Makefile $@ || exit; \
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
