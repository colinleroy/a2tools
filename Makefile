ifdef IIGS
iigs_CFLAGS := -DIIGS
suffix := -iigs
RBROWSER_BIN := 
else
ifdef OLDII
suffix := -6502
WOZAMP_VIDEOPLAY := 1
RBROWSER_BIN := src/wozamp/rbrowser.bin
else
iigs_FLAGS :=
suffix := -65c02
WOZAMP_VIDEOPLAY := 1
RBROWSER_BIN := 
endif
endif

SUBDIRS = src doc

stp_disk_PROGS = \
	src/stp/stp.bin

ifdef WOZAMP_VIDEOPLAY
wozamp_disk_PROGS = \
	src/wozamp/wozamp.bin \
	$(RBROWSER_BIN) \
	src/wozamp/videoplay.bin
else
wozamp_disk_PROGS = \
	src/wozamp/wozamp.bin \
	$(RBROWSER_BIN)
endif

weather_disk_PROGS = \
	src/weather/weather.bin

ammonoid_disk_PROGS = \
	src/ammonoid/ammonoid.bin

woztubes_disk_PROGS = \
	src/woztubes/woztubes.bin

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

ammonoid$(suffix).po: $(ammonoid_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ AMMONOID
	java -jar bin/ac.jar -p $@ AMMONOID.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	mkdir -p dist && cp $@ dist/; \

woztubes$(suffix).po: $(woztubes_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ WOZTUBES
	java -jar bin/ac.jar -p $@ WOZTUBES.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	mkdir -p dist && cp $@ dist/; \

woztubesperso$(suffix).po: $(woztubes_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ WOZTUBES
	java -jar bin/ac.jar -p $@ WOZTUBES.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ STPSTARTURL TXT < src/woztubes/STPSTARTURL; \
	java -jar bin/ac.jar -p $@ CLISETTINGS TXT < src/woztubes/CLISETTINGS; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done

wozamp$(suffix).po: $(wozamp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ WOZAMP
	java -jar bin/ac.jar -p $@ WOZAMP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	mkdir -p dist && cp $@ dist/; \

wozampperso$(suffix).po: $(wozamp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ WOZAMP
	java -jar bin/ac.jar -p $@ WOZAMP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ STPSTARTURL TXT < src/wozamp/STPSTARTURL; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done

weather$(suffix).po: $(weather_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ WEATHER
	java -jar bin/ac.jar -p $@ WEATHER.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	mkdir -p dist && cp $@ dist/; \

stp$(suffix).po: $(stp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ STP
	java -jar bin/ac.jar -p $@ STP.SYSTEM SYS < bin/loader.system; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	mkdir -p dist && cp $@ dist/; \

stpperso$(suffix).po: $(stp_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ STP
	java -jar bin/ac.jar -p $@ STP.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ STPSTARTURL TXT < src/stp/STPSTARTURL; \
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
	mkdir -p dist && cp $@ dist/; \

homectrl$(suffix).po: $(homectrl_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ HOMECONTROL
	java -jar bin/ac.jar -p $@ HOMECTRL.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done

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
	mkdir -p dist && cp $@ dist/; \

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
	mkdir -p dist && cp $@ dist/; \

imageviewer$(suffix).po: $(imageviewer_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -n $@ IMGVIEW
	java -jar bin/ac.jar -p $@ IMGVIEW.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -d $@ BASIC.SYSTEM; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	mkdir -p dist && cp $@ dist/; \

doc-dist:
	$(MAKE) -C doc -f Makefile dist

ifndef IIGS
ifdef OLDII
#6502 things
dist: all \
	ammonoid$(suffix).po \
	stp$(suffix).po \
	mastodon$(suffix).po \
	wozamp$(suffix).po \
	woztubes$(suffix).po \
	weather$(suffix).po

else
#65c02 things
dist: all \
	ammonoid$(suffix).po \
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
	woztubes$(suffix).po \
	weather$(suffix).po \
	doc-dist
endif

else
#IIgs things
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
	weather$(suffix).po
endif
