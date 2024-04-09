ifdef IIGS
iigs_CFLAGS := -DIIGS
suffix := -iigs
else
ifdef OLDII
suffix := -oldii
else
iigs_FLAGS :=
suffix :=
WOZAMP_VIDEOPLAY := 1
endif
endif

SUBDIRS = src doc

stp_disk_PROGS = \
	src/stp/stp.bin

ifdef WOZAMP_VIDEOPLAY
wozamp_disk_PROGS = \
	src/wozamp/wozamp.bin \
	src/wozamp/videoplay.bin
else
wozamp_disk_PROGS = \
	src/wozamp/wozamp.bin
endif

telnet_disk_PROGS = \
	src/telnet/telnet.bin

homectrl_disk_PROGS = \
	src/homecontrol-client/homectrl.bin \
	src/homecontrol-client/grphview.bin

mastodon_disk_PROGS = \
	src/mastodon/mastodon.bin \
	src/mastodon/mastocli.bin \
	src/mastodon/mastowrite.bin \
	src/mastodon/mastoimg.bin

quicktake_disk_PROGS = \
	src/quicktake/slowtake.bin \
	src/quicktake/qtktconv.bin \
	src/quicktake/qtknconv.bin \
	src/quicktake/jpegconv.bin \
	src/image-viewer/imgview.bin

quicktake_disk_IMGS = \
	src/quicktake/about

imageviewer_disk_PROGS = \
	src/image-viewer/imgview.bin

CLEANDISK = disks/basic-empty.dsk

.PHONY: all clean

clean:
	rm -f *$(suffix).dsk && \
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

all upload:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

wozamp$(suffix).dsk: $(wozamp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ WOZAMP
	java -jar bin/ac.jar -p $@ WOZAMP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

wozampperso$(suffix).dsk: $(wozamp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ WOZAMP
	java -jar bin/ac.jar -p $@ WOZAMP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ STPSTARTURL TXT < src/wozamp/STPSTARTURL; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/;

stp$(suffix).dsk: $(stp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ STP
	java -jar bin/ac.jar -p $@ STP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

stpperso$(suffix).dsk: $(stp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ STP
	java -jar bin/ac.jar -p $@ STP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ STPSTARTURL TXT < src/stp/STPSTARTURL; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/;

telnet$(suffix).dsk: $(telnet_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ TELNET
	java -jar bin/ac.jar -p $@ TELNET.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

homectrl$(suffix).dsk: $(homectrl_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ HOMECONTROL
	java -jar bin/ac.jar -p $@ HOMECTRL.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

homectrlperso$(suffix).dsk: $(homectrl_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ HOMECONTROL
	java -jar bin/ac.jar -p $@ HOMECTRL.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ SRVURL TXT < src/homecontrol-client/SRVURL; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/;

mastoperso$(suffix).dsk: $(mastodon_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ MASTODON
	java -jar bin/ac.jar -p $@ MASTODON.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ mastsettings TXT < src/mastodon/mastsettings; \
	java -jar bin/ac.jar -p $@ clisettings TXT < src/mastodon/clisettings; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/;

mastodon$(suffix).dsk: $(mastodon_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ MASTODON
	java -jar bin/ac.jar -p $@ MASTODON.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

quicktake$(suffix).dsk: $(quicktake_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ QUICKTAKE
	java -jar bin/ac.jar -p $@ SLOWTAKE.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	for img in $(quicktake_disk_IMGS); do \
		java -jar bin/ac.jar -p $@ $$(basename $$img) BIN 0x2000 < $$img; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

imageviewer$(suffix).dsk: $(imageviewer_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ QUICKTAKE
	java -jar bin/ac.jar -p $@ IMGVIEW.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

doc-dist:
	$(MAKE) -C doc -f Makefile dist

dist: all \
	stp$(suffix).dsk \
	telnet$(suffix).dsk \
	homectrl$(suffix).dsk \
	imageviewer$(suffix).dsk \
	mastodon$(suffix).dsk \
	mastoperso$(suffix).dsk \
	homectrlperso$(suffix).dsk \
	stpperso$(suffix).dsk \
	quicktake$(suffix).dsk \
	wozamp$(suffix).dsk \
	doc-dist
