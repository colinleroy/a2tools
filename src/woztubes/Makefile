fCC65_TARGET = apple2enh
ifdef IIGS
serial_hw := z8530
iigs_suffix := -iigs
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
upload_subdir := /iigs
else
ifdef OLDII
serial_hw := acia
iigs_FLAGS :=
suffix :=
upload_subdir :=
CC65_TARGET := apple2
STREAMER_VER := 6502
START_ADDR := 0x113A
else
serial_hw := acia
iigs_suffix :=
iigs_CFLAGS :=
CC65_TARGET = apple2enh
trgt = apple2c0
STREAMER_VER := 65c02
START_ADDR := 0x113A
endif
endif

USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -I ../stp -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror -DSTP_DATA_SIZE=8192

.PHONY: all clean

woztubes_SOURCES := \
	../lib/surl/surl_stream_av/$(STREAMER_VER).s \
	main.c \
	video_providers.c \
	config.c \
	../lib/scroll.s \
	../lib/dputc.s \
	../lib/dgets.c \
	../lib/FVTABZ.s \
	../lib/strsplit.c \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/serial/$(serial_hw)/common.s \
	../lib/serial/$(serial_hw)/no_irq.s \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_get_json.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/asm/hgr.s \
	../lib/surl/surl_ping.c \
	../lib/extrazp.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s \
	#../lib/fastirq$(iigs_suffix).s \

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

woztubes_CLCFLAGS = -t $(CC65_TARGET) -I ../lib \
	-DBUF_1K_ADDR=0x800 -DBUF_1K_SIZE=0x3FF \
	-DBUF_8K_ADDR=0x4000 -DBUF_8K_SIZE=0x1FFF \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-woztubes.cfg \
	-vm -m woztubes.map $(USE_LANGCARD) -O --start-addr $(START_ADDR)

all: \
	splash.h \
	woztubes.bin

clean:
	rm -f *.bin *.o *.map splash.h splash.hgr

woztubes.hgr:
	../surl-server/hgr-convert woztubes.png

splash.h: woztubes.hgr
	../../bin/make-splash.sh woztubes.png splash.h

woztubes.bin: $(woztubes_CL65SOURCES) $(woztubes_SOURCES)
	cl65 -g -Wl --dbgfile,woztubes.dbg -Ln woztubes.lbl $(CFLAGS) $(iigs_CFLAGS) $(woztubes_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/woztubes/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/woztubes/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/woztubes/
