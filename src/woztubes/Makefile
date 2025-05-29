TARGET := apple2
CPU := 6502

GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	-DBUF_1K_ADDR=0x800 -DBUF_1K_SIZE=0x3FF \
	-DBUF_8K_ADDR=0x4000 -DBUF_8K_SIZE=0x1FFF \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \

START_ADDR := 0x113A

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -I ../stp -I . -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror -DSTP_DATA_SIZE=8192

.PHONY: all clean

woztubes_SOURCES := \
	../lib/surl/surl_stream_av/$(CPU).s \
	main.c \
	video_providers.c \
	config.c \
	../lib/scroll.s \
	../lib/dputc.s \
	../lib/dget_text_single.c \
	../lib/FVTABZ.s \
	../lib/strsplit.c \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_get_json.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/asm/hgr.s \
	../lib/asm/backup_hgrpage.s \
	../lib/asm/hgr_setup_mono40.s \
	../lib/asm/hgr_mono40.s \
	../lib/surl/asm/surl_ping.s \
	../lib/extrazp.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s \
	../lib/a2_features.s \
	../lib/oa_cgetc.s \
	../lib/charsets.c \

### Warning! some memory tricks happen.

#Carefully consider the life-cycle of any data put in there as it will be
#overwritten at some point!

#0x800-0xC00 is reserved for file iobuf via iobuf-0800.o,
#0x800-0xC00 is ALSO a 1k buffer for network I/O,
#0x800-0xC00 is ALSO the text page 2 (and will be overwritten by subtitles)

#0x4000-0x6000 is HGR page 2,
#0x4000-0x6000 is ALSO an 8k buffer for network I/O,
#0x4000-0x6000 is ALSO used to store setup code in RT_ONCE in order to shrink
#              the binary a bit. But DO NOT runtime_once_clean() as usual!

woztubes_CLCFLAGS = -t $(TARGET) -I ../lib -I . \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-woztubes.cfg \
	-vm -m woztubes.map $(GENERAL_CFLAGS) -O --start-addr $(START_ADDR)

all: woztubes.po

CLEANDISK = ../../disks/ProDOS_2_4_3.po
woztubes.po: splash.h woztubes.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ WOZTUBES
	java -jar ../../bin/ac.jar -p $@ WOZTUBES.SYSTEM SYS < ../../bin/loader.system; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

woztubesperso.po: woztubes.po
	cp $< $@; \
	java -jar ../../bin/ac.jar -p $@ STPSTARTURL TXT < STPSTARTURL; \


clean:
	rm -f *.bin *.o *.map splash*.h *.hgr *.po *.dbg *.lbl

woztubes.hgr: woztubes.png
	../surl-server/hgr-convert $<

splash.h: woztubes.hgr
	../../bin/make-splash.sh woztubes.png splash.h

woztubes.bin: $(woztubes_CL65SOURCES) $(woztubes_SOURCES)
	cl65 -g -Wl --dbgfile,woztubes.dbg -Ln woztubes.lbl $(CFLAGS) $(woztubes_CLCFLAGS) -o $@ $^  $(TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/woztubes/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/woztubes/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/woztubes/

dist: woztubes.po
	mkdir -p ../../dist/
	cp $< ../../dist/
