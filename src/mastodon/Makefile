TARGET := apple2enh
USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

ifdef IIGS
serial_hw_SOURCES := ../lib/serial/z8530/common.s ../lib/serial/z8530/no_irq.s
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs
else
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
ifdef OLDII
iigs_FLAGS :=
suffix :=
upload_subdir :=
TARGET := apple2
else
iigs_FLAGS :=
suffix :=
upload_subdir :=
endif
endif
common_SOURCES := \
	api/common.c \
	../lib/dgets.c \
	../lib/path_helper.c \
	../lib/scrollwindow.c \
	../lib/serial/simple_serial.c \
	../lib/malloc0.c \
	../lib/strsplit.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_get_json.c \
	../lib/surl/surl_send.c

common_CC65SOURCES := \
	../lib/surl/surl_read_with_barrier.s \
	../lib/serial/serial_read_no_irq.s \
	../lib/serial/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	../lib/clrzone.s \
	../lib/dputs.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/fastirq$(suffix).s \
	 ../lib/extrazp.s \
	../lib/runtime_once_clean.s

mastodon_SOURCES := \
	$(common_SOURCES) \
	cli/login.c \
	cli/logo.c \
	api/oauth.c \
	cli/config.c \
	cli/logo.c \
	../lib/surl/surl_find_line.c \
	../lib/surl/surl_ping.c \
	../lib/serial/simple_serial_configure.c \
	../lib/path_helper.c \
	../lib/scrollwindow.c \
	../lib/malloc0.c

mastodon_CC65SOURCES := \
	$(common_CC65SOURCES)

mastoimg_SOURCES := \
	api/common.c \
	cli/img.c \
	api/media.c \
	../lib/serial/simple_serial.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_get_json.c \
	../lib/path_helper.c \
	../lib/file_select.c \
	../lib/strsplit.c \
	../lib/malloc0.c \
	../lib/realloc_safe.c \
	../lib/hgr.c \
	../lib/scrollwindow.c \
	../lib/dgets.c \

mastoimg_CC65SOURCES := \
	../lib/surl/surl_read_with_barrier.s \
	../lib/serial/serial_read_no_irq.s \
	../lib/serial/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	../lib/clrzone.s \
	../lib/FVTABZ.s \
	../lib/fastirq$(suffix).s \
	 ../lib/extrazp.s \
	../lib/runtime_once_clean.s \
	../lib/surl/surl_stream_video.s \
	../lib/surl/surl_wait_for_stream.c \
	../lib/dputc.s \
	../lib/scroll.s
	
mastocli_SOURCES := \
	$(common_SOURCES) \
	cli/header.c \
	cli/print.c \
	cli/tl.c\
	api/api.c \
	api/account.c \
	api/notification.c \
	api/status.c \
	api/poll.c \
	../lib/progress_bar.c \
	../lib/path_helper.c

mastocli_CC65SOURCES := \
	$(common_CC65SOURCES) \

mastowrite_SOURCES := \
	$(common_SOURCES) \
	cli/compose.c \
	cli/compose_header.c \
	cli/print.c \
	api/account.c \
	api/compose.c \
	api/media.c \
	api/status.c \
	api/poll.c \
	../lib/file_select.c \
	../lib/progress_bar.c \
	../lib/path_helper.c \
	../lib/realloc_safe.c \

mastowrite_CC65SOURCES := \
	$(common_CC65SOURCES) \

GCCSOURCES := \
	../lib/extended_conio.c \
	../lib/clrzone.c \
	../lib/strtrim.c \
	../lib/tgi_sdl.c \
	../lib/tgi_fastline.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_io.c

DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -I api -I cli -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x400 $(USE_LANGCARD) -C ../../config/apple2enh-rtonce.cfg $(debug_CFLAGS)  $(iigs_CFLAGS)
#Using start address 0x4000 is easier than HGR page 2, and also allows to use 80 columns

mastodon_CLCFLAGS = $(CLCFLAGS)
mastocli_CLCFLAGS = $(CLCFLAGS)
mastoimg_CLCFLAGS = $(CLCFLAGS) -DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER --start-addr 0x6000
mastowrite_CLCFLAGS = $(CLCFLAGS) -DDGETS_MULTILINE

GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: mastodon.bin mastodon.x86_64 \
	mastocli.bin mastocli.x86_64 \
	mastowrite.bin mastowrite.x86_64 \
	mastoimg.bin mastoimg.x86_64

mastodon.bin: $(mastodon_SOURCES) $(mastodon_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastodon.dbg" $(CFLAGS) $(mastodon_CLCFLAGS) -vm -m mastodon.map -o $@ $^
mastocli.bin: $(mastocli_SOURCES) $(mastocli_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastocli.dbg" $(CFLAGS) $(mastocli_CLCFLAGS) -vm -m mastocli.map -o $@ $^
mastowrite.bin: $(mastowrite_SOURCES) $(mastowrite_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastowrite.dbg" $(CFLAGS) $(mastowrite_CLCFLAGS) -vm -m mastowrite.map -o $@ $^
mastoimg.bin: $(mastoimg_SOURCES) $(mastoimg_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastoimg.dbg" $(CFLAGS) $(mastoimg_CLCFLAGS) -vm -m mastoimg.map -o $@ $^

mastodon.x86_64: $(mastodon_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastocli.x86_64: $(mastocli_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastowrite.x86_64: $(mastowrite_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastoimg.x86_64: $(mastoimg_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f *.bin *.x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/mastodon/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/mastodon/ && \
	cp masto*.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/mastodon/
