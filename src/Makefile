
ifdef IIGS
UPLOAD_SUBDIRS = homecontrol-client telnet image-viewer weather 

else
ifdef OLDII
UPLOAD_SUBDIRS = homecontrol-client weather

else
UPLOAD_SUBDIRS = homecontrol-client telnet image-viewer weather

endif
endif
SUBDIRS = a2trace surl-server $(UPLOAD_SUBDIRS)
INSTALL_SUBDIRS = a2trace surl-server

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
