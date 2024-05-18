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

CLEANDISK = disks/ProDOS_2_4_3.po

.PHONY: all clean

clean:
	rm -f *$(suffix).po && \
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

all upload:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

wozamp$(suffix).po: $(wozamp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ WOZAMP
	java -jar bin/ac.jar -p $@ WOZAMP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ dist/; \

wozampperso$(suffix).po: $(wozamp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ WOZAMP
	java -jar bin/ac.jar -p $@ WOZAMP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ STPSTARTURL TXT < src/wozamp/STPSTARTURL; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done

stp$(suffix).po: $(stp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ STP
	java -jar bin/ac.jar -p $@ STP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ dist/; \

stpperso$(suffix).po: $(stp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ STP
	java -jar bin/ac.jar -p $@ STP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ STPSTARTURL TXT < src/stp/STPSTARTURL; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done

telnet$(suffix).po: $(telnet_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ TELNET
	java -jar bin/ac.jar -p $@ TELNET.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ dist/; \

homectrl$(suffix).po: $(homectrl_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ HOMECONTROL
	java -jar bin/ac.jar -p $@ HOMECTRL.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ dist/; \

homectrlperso$(suffix).po: $(homectrl_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ HOMECONTROL
	java -jar bin/ac.jar -p $@ HOMECTRL.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ SRVURL TXT < src/homecontrol-client/SRVURL; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done

mastoperso$(suffix).po: $(mastodon_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ MASTODON
	java -jar bin/ac.jar -p $@ MASTODON.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ mastsettings TXT < src/mastodon/mastsettings; \
	java -jar bin/ac.jar -p $@ clisettings TXT < src/mastodon/clisettings; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done

mastodon$(suffix).po: $(mastodon_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ MASTODON
	java -jar bin/ac.jar -p $@ MASTODON.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ dist/; \

quicktake$(suffix).po: $(quicktake_disk_PROGS)
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
	cp $@ dist/; \

imageviewer$(suffix).po: $(imageviewer_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ QUICKTAKE
	java -jar bin/ac.jar -p $@ IMGVIEW.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ dist/; \

doc-dist:
	$(MAKE) -C doc -f Makefile dist

dist: all \
	stp$(suffix).po \
	telnet$(suffix).po \
	homectrl$(suffix).po \
	imageviewer$(suffix).po \
	mastodon$(suffix).po \
	mastoperso$(suffix).po \
	homectrlperso$(suffix).po \
	stpperso$(suffix).po \
	quicktake$(suffix).po \
	wozamp$(suffix).po \
	doc-dist
