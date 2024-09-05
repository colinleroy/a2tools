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
BUILD_VIDEOPLAY := 1
STREAMER_VER := 6502
STREAM_URL_SEGMENT := "CODE"
VIDEOPLAY_START_ADDR := 0x1E00
else
serial_hw := acia
iigs_suffix :=
iigs_CFLAGS :=
CC65_TARGET = apple2enh
trgt = apple2c0
BUILD_VIDEOPLAY := 1
STREAMER_VER := 65c02
STREAM_URL_SEGMENT := "LOWCODE"
VIDEOPLAY_START_ADDR := 0x1F00
endif
endif

wozamp_SOURCES := \
	main.c \
	config.c \
	backup_logo.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_configure.c \
	../lib/path_helper.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_receive.c \
	../lib/surl/surl_ping.c \
	../lib/malloc0.c \
	../lib/dgets.c \
	../lib/scrollwindow.c \
	../lib/strsplit.c \
	../stp/stp_list.c

wozamp_CL65SOURCES = \
	../lib/surl/surl_stream_audio.s \
	../lib/hgr.c \
	../lib/surl/surl_read_with_barrier.s \
	../lib/serial/simple_serial_read.s \
	../lib/serial/simple_serial_io.s \
	../lib/serial/$(serial_hw)/common.s \
	../lib/serial/$(serial_hw)/control.s \
	../lib/serial/$(serial_hw)/no_irq.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/fastirq$(suffix).s \
	../lib/runtime_once_clean.s \
	../lib/extrazp.s \
	../lib/clrzone.s \
	../lib/platform_sleep.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s

wozamp_CLCFLAGS = -t $(CC65_TARGET) -DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-DSTP_DATA_SIZE=8192 \
	-I ../lib -I ../stp \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-pwmstreamer.cfg \
	-vm -m wozamp.map -DSURL_TO_LANGCARD -O

rbrowser_SOURCES := \
	radio-browser.c \
	config.c \
	backup_logo.c \
	../lib/serial/simple_serial.c \
	../lib/path_helper.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_get_json.c \
	../lib/malloc0.c \
	../lib/dgets.c \
	../lib/scrollwindow.c \
	../lib/strsplit.c

rbrowser_CL65SOURCES = \
	../lib/surl/surl_stream_audio.s \
	../lib/hgr.c \
	../lib/surl/surl_read_with_barrier.s \
	../lib/serial/simple_serial_read.s \
	../lib/serial/simple_serial_io.s \
	../lib/serial/$(serial_hw)/common.s \
	../lib/serial/$(serial_hw)/control.s \
	../lib/serial/$(serial_hw)/no_irq.s \
	../lib/dputc.s \
	../lib/dputs.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/fastirq$(suffix).s \
	../lib/runtime_once_clean.s \
	../lib/extrazp.s \
	../lib/clrzone.s

rbrowser_CLCFLAGS = -t $(CC65_TARGET) \
	-I ../lib -I ../stp \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-rbrowser.cfg \
	-vm -m rbrowser.map -DSURL_TO_LANGCARD -O --start-addr 0x4000


.PHONY: all clean

### Warning! some memory tricks happen.

#0x4000-0x6000 is HGR page 2,
#0x4000-0x6000 is ALSO used to store setup code in RT_ONCE in order to shrink
#              the binary a bit. But DO NOT runtime_once_clean() as usual!

ifdef BUILD_VIDEOPLAY
videoplay_SOURCES := \
	../lib/surl/surl_stream_av/$(STREAMER_VER).s \
	../lib/surl/surl_stream_av/stream_url.c \
	videoplay.c \
	../lib/hgr.c \
	../lib/scrollwindow.c \
	../lib/FVTABZ.s \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_io.s \
	../lib/serial/$(serial_hw)/common.s \
	../lib/serial/$(serial_hw)/control.s \
	../lib/serial/$(serial_hw)/no_irq.s \
	../lib/path_helper.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_read_with_barrier.s \
	../lib/fastirq$(iigs_suffix).s \
	../lib/extrazp.s \
	../lib/malloc0.c \

videoplay_CLCFLAGS = -t $(CC65_TARGET) \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-DSTREAM_URL_SEGMENT='$(STREAM_URL_SEGMENT)' \
	-I ../lib \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-wozamp-videoplay-$(STREAMER_VER).cfg \
	-vm -m videoplay.map -DSURL_TO_LANGCARD -O --start-addr $(VIDEOPLAY_START_ADDR)

all: \
	splash.h \
	splash-video.h \
	wozamp.bin \
	rbrowser.bin \
	videoplay.bin
else

all: \
	splash.h \
	wozamp.bin \
	rbrowser.bin \

endif

clean:
	rm -f *.bin *.o *.map splash.h splash-video.h splash.hgr splash-video.hgr

wozamp.hgr:
	../surl-server/hgr-convert wozamp.png

wozamp-video.hgr:
	../surl-server/hgr-convert wozamp-video.png

splash.h: wozamp.hgr
	../../bin/make-splash.sh wozamp.png splash.h

splash-video.h: wozamp-video.hgr
	../../bin/make-splash.sh wozamp-video.png splash-video.h

wozamp.bin: $(wozamp_CL65SOURCES) $(wozamp_SOURCES)
	cl65 -g -Wl --dbgfile,wozamp.dbg -Ln wozamp.lbl $(CFLAGS) $(iigs_CFLAGS) $(wozamp_CLCFLAGS) -o $@ $^

rbrowser.bin: $(rbrowser_CL65SOURCES) $(rbrowser_SOURCES)
	cl65 -g -Wl --dbgfile,rbrowser.dbg -Ln rbrowser.lbl $(CFLAGS) $(iigs_CFLAGS) $(rbrowser_CLCFLAGS) -o $@ $^

videoplay.bin: $(videoplay_CL65SOURCES) $(videoplay_SOURCES)
	cl65 -g -Wl --dbgfile,videoplay.dbg -Ln videoplay.lbl $(CFLAGS) $(iigs_CFLAGS) $(videoplay_CLCFLAGS) -o $@ $^

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/wozamp/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/wozamp/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/wozamp/
