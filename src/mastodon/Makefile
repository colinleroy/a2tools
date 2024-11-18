TARGET := apple2enh
USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

ifdef IIGS
serial_hw_SOURCES := ../lib/serial/z8530/common.s ../lib/serial/z8530/no_irq.s
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs
CPU=65c02
else
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
ifdef OLDII
iigs_FLAGS :=
suffix :=
upload_subdir :=
TARGET := apple2
CPU=6502
else
iigs_FLAGS :=
suffix :=
upload_subdir :=
CPU=65c02
endif
endif

common_CC65SOURCES := \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_get_json.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/asm/scrollwindow.s \
	../lib/dputs.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	 ../lib/extrazp.s \
	 #../lib/fastirq$(suffix).s \

mastodon_SOURCES := \
	api/common.c \
	cli/login.c \
	cli/logo.c \
	api/oauth.c \
	cli/config.c \
	cli/logo.c \
	../lib/dgets.c \
	../lib/strsplit.c \
	../lib/charsets.c

mastodon_CC65SOURCES := \
	$(common_CC65SOURCES) \
	../lib/surl/asm/surl_send.s \
	../lib/surl/asm/surl_find_line.s \
	../lib/surl/asm/surl_ping.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/runtime_once_clean.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s

mastoimg_SOURCES := \
	api/common.c \
	cli/img.c \
	api/media.c \
	../lib/file_select.c \
	../lib/realloc_safe.c \
	../lib/strsplit.c \
	../lib/dgets.c

ifdef IIGS

mastoimg_CC65SOURCES := \
	../lib/surl/asm/surl_stream_video.s

else

mastoimg_CC65SOURCES := \
	../lib/surl/surl_stream_av/$(CPU).s

endif

mastoimg_CC65SOURCES := \
	$(mastoimg_CC65SOURCES) \
	$(common_CC65SOURCES) \
	../lib/asm/hgr.s \
	../lib/vsdrive.s

mastocli_SOURCES := \
	api/common.c \
	cli/header.c \
	cli/print.c \
	cli/tl.c\
	api/api.c \
	api/account.c \
	api/account_new_from_json.c \
	api/notification.c \
	api/status.c \
	api/poll.c \
	api/poll_update_vote.c \
	api/date_format.c \
	../lib/dgets.c \
	../lib/strsplit.c \
	../lib/progress_bar.c

mastocli_CC65SOURCES := \
	$(common_CC65SOURCES) \
	../lib/surl/asm/surl_send.s \
	../lib/asm/atoc.s \
	../lib/runtime_once_clean.s

mastowrite_SOURCES := \
	api/common.c \
	cli/compose.c \
	cli/compose_header.c \
	cli/print.c \
	api/account.c \
	api/compose.c \
	api/media.c \
	api/status.c \
	api/poll.c \
	api/date_format.c \
	../lib/dgets.c \
	../lib/strsplit.c \
	../lib/file_select.c \
	../lib/progress_bar.c \
	../lib/realloc_safe.c \

mastowrite_CC65SOURCES := \
	$(common_CC65SOURCES) \
	../lib/surl/asm/surl_send.s \
	../lib/asm/atoc.s \
	../lib/runtime_once_clean.s \
	../lib/vsdrive.s

GCCSOURCES := \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/surl/c/surl_core.c \
	../lib/surl/c/surl_ping.c \
	../lib/surl/c/surl_find_line.c \
	../lib/surl/c/surl_send.c \
	../lib/surl/c/surl_get_json.c \
	../lib/c/scrollwindow.c \
	../lib/c/clrzone.c \
	../lib/c/path_helper.c \
	../lib/c/malloc0.c \
	../lib/c/hgr.c \
	../lib/c/atoc.c \
	../lib/extended_conio.c \
	../lib/strtrim.c \
	../lib/tgi_sdl.c \
	../lib/tgi_fastline.c

DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -I api -I cli -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x400 $(USE_LANGCARD) $(debug_CFLAGS)  $(iigs_CFLAGS)
#Using start address 0x4000 is easier than HGR page 2, and also allows to use 80 columns

mastodon_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-rtonce.cfg
mastocli_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-rtonce.cfg
ifdef OLDII
mastoimg_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-mastoimg-6502.cfg \
	-DSTART_ADDR=0x3EA0 --start-addr 0x3EA0
else
mastoimg_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-mastoimg-65c02.cfg \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-DSTART_ADDR=0x5EA0 --start-addr 0x5EA0
endif
mastowrite_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-rtonce.cfg -DDGETS_MULTILINE

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
mastoimg.bin: $(mastoimg_CC65SOURCES) $(mastoimg_SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastoimg.dbg" $(CFLAGS) $(mastoimg_CLCFLAGS) -vm -m mastoimg.map -o $@ $^ $(CC65_TARGET)-iobuf-0800.o

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
