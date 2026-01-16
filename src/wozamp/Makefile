TARGET := apple2
CPU := 6502
WOZAMP_START_ADDR := 0xC03
VIDEOPLAY_START_ADDR := 0x1D00

GENERAL_CFLAGS := -DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	-DDYN_BOX_DRAW -DAPPLE2_INCLUDE_IIE_CHARS \

FLOP_NAME = WOZAMP
CLEANDISK = ../../disks/ProDOS_2_4_3.po

wozamp_SOURCES := \
	main.c \
	config.c \
	radio-browser.c \
	../lib/surl/asm/surl_get_json.s \
	../lib/dget_text_single.c \
	../lib/strsplit.c \
	../stp/stp_list.c \
	../lib/charsets.c

wozamp_CL65SOURCES = \
	../lib/surl/surl_stream_audio/$(CPU).s \
	../lib/surl/surl_stream_audio/backup_audiocode.s \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_ping.s \
	../lib/surl/asm/surl_receive.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/surl/asm/surl_read_image.s \
	../lib/a2_features.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/malloc0.s \
	../lib/asm/realloc_safe.s \
	../lib/asm/hgr.s \
	../lib/asm/backup_hgrpage.s \
	../lib/asm/reserve_auxhgr.s \
	../lib/asm/unlink_auxhgr.s \
	../lib/asm/auxhgr_file.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/runtime_once_clean.s \
	../lib/extrazp.s \
	../lib/asm/clrzone.s \
	../lib/platform_sleep.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s \

wozamp_CLCFLAGS = -t $(TARGET) -DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-DSTP_DATA_SIZE=8192 \
	-I ../lib -I ../stp -I . \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-pwmstreamer.cfg \
	-vm -m wozamp.map $(GENERAL_CFLAGS) --start-addr $(WOZAMP_START_ADDR) -O

.PHONY: all clean

### Warning! some memory tricks happen.

#0x4000-0x6000 is HGR page 2,
#0x4000-0x6000 is ALSO used to store setup code in RT_ONCE in order to shrink
#              the binary a bit. But DO NOT runtime_once_clean() as usual!

all dist: wozamp.po
perso: all wozampperso.po

videoplay_SOURCES := \
	../lib/surl/surl_stream_av/$(CPU).s \
	videoplay.c \
	../lib/FVTABZ.s \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/malloc0.s \
	../lib/asm/hgr.s \
	../lib/asm/hgr_setup_mono40.s \
	../lib/asm/auxhgr_file.s \
	../lib/asm/hgr_mono40.s \
	../lib/a2_features.s \
	../lib/extrazp.s

videoplay_CLCFLAGS = -t $(TARGET) \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-I ../lib -I . \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-wozamp-videoplay.cfg \
	-vm -m videoplay.map $(GENERAL_CFLAGS) -O --start-addr $(VIDEOPLAY_START_ADDR)

DISK_PROGS = splash.h splash-video.h wozamp.bin videoplay.bin

add_disk:
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)WOZAMP.SYSTEM SYS < ../../bin/loader.system; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $(IMG) $(PREFIX)$$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

wozamp.po: $(DISK_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ $(FLOP_NAME)
	IMG=$@ PREFIX= make add_disk

hdv: 
	java -jar ../../bin/ac.jar -d $(IMG) $(FLOP_NAME)
	PREFIX=$(FLOP_NAME)/ make add_disk

wozampperso.po: wozamp.po
	cp $< $@; \
	java -jar ../../bin/ac.jar -p $@ STPSTARTURL TXT < STPSTARTURL; \

clean:
	rm -f *.bin *.o *.map splash.h splash-video.h splash.hgr splash-video.hgr *.lbl *.dbg *.po *.hgr

wozamp.hgr: wozamp.png
	../surl-server/hgr-convert $<

wozamp-video.hgr: wozamp-video.png
	../surl-server/hgr-convert $<

splash.h: wozamp.hgr
	../../bin/make-splash.sh wozamp.png splash.h

splash-video.h: wozamp-video.hgr
	../../bin/make-splash.sh wozamp-video.png splash-video.h

wozamp.bin: $(wozamp_CL65SOURCES) $(wozamp_SOURCES)
	cl65 -g -Wl --dbgfile,wozamp.dbg -Ln wozamp.lbl $(CFLAGS) $(wozamp_CLCFLAGS) -o $@ $^ $(TARGET)-iobuf-0800.o

videoplay.bin: $(videoplay_CL65SOURCES) $(videoplay_SOURCES)
	cl65 -g -Wl --dbgfile,videoplay.dbg -Ln videoplay.lbl $(CFLAGS) $(videoplay_CLCFLAGS) -o $@ $^

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/wozamp/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/wozamp/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/wozamp/
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/

dist: wozamp.po
	mkdir -p ../../dist/
	cp $< ../../dist/
