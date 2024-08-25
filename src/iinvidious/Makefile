CC65_TARGET = apple2enh
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
START_ADDR := 0xC3A
else
serial_hw := acia
iigs_suffix :=
iigs_CFLAGS :=
CC65_TARGET = apple2enh
trgt = apple2c0
STREAMER_VER := 65c02
START_ADDR := 0x1F3A
endif
endif

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -I ../stp -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror -DSTP_DATA_SIZE=8192

.PHONY: all clean

iinvidious_SOURCES := \
	../lib/surl/surl_stream_av/$(STREAMER_VER).s \
	main.c \
	videoplay.c \
	config.c \
	../lib/hgr.c \
	../lib/clrzone.s \
	../lib/scrollwindow.c \
	../lib/scroll.s \
	../lib/dputc.s \
	../lib/dgets.c \
	../lib/FVTABZ.s \
	../lib/strsplit.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_configure.c \
	../lib/serial/simple_serial_io.s \
	../lib/serial/$(serial_hw)/common.s \
	../lib/serial/$(serial_hw)/control.s \
	../lib/serial/$(serial_hw)/no_irq.s \
	../lib/path_helper.c \
	../lib/surl/surl_ping.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_get_json.c \
	../lib/surl/surl_read_with_barrier.s \
	../lib/fastirq$(iigs_suffix).s \
	../lib/extrazp.s \
	../lib/malloc0.c \

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

iinvidious_CLCFLAGS = -t $(CC65_TARGET) -I ../lib \
	-DBUF_1K_ADDR=0x800 -DBUF_1K_SIZE=0x3FF \
	-DBUF_8K_ADDR=0x4000 -DBUF_8K_SIZE=0x1FFF \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-DSIMPLE_SERIAL_DUMP --asm-define SIMPLE_SERIAL_DUMP \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-iinvidious-$(STREAMER_VER).cfg \
	-vm -m iinvidious.map -DSURL_TO_LANGCARD -O --start-addr $(START_ADDR)

all: \
	splash.h \
	iinvid.bin

clean:
	rm -f *.bin *.o *.map

iinvidious.hgr:
	hgr-convert iinvidious.png

splash.h: iinvidious.hgr
	../../bin/make-splash.sh iinvidious.png splash.h

iinvid.bin: $(iinvidious_CL65SOURCES) $(iinvidious_SOURCES)
	cl65 -g -Wl --dbgfile,iinvidious.dbg -Ln iinvidious.lbl $(CFLAGS) $(iigs_CFLAGS) $(iinvidious_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/iinvidious/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/iinvidious/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/iinvidious/
