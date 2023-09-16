ifdef SER_DEBUG
ser_debug_SOURCES := ../lib/simple_serial_ext.c
debug_CFLAGS := -DSER_DEBUG=1
else
ser_debug_SOURCES :=
debug_CFLAGS :=
endif

ifdef IIGS
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs

else
iigs_FLAGS :=
suffix :=
upload_subdir :=
endif

common_SOURCES := \
	api/common.c \
	../lib/clrzone.c \
	../lib/dgets.c \
	../lib/path_helper.c \
	../lib/scrollwindow.c \
	../lib/simple_serial.c \
	$(ser_debug_SOURCES) \
	../lib/strsplit.c \
	../lib/surl_core.c \
	../lib/surl_get_json.c \
	../lib/surl_send.c

mastodon_SOURCES := \
	$(common_SOURCES) \
	cli/login.c \
	cli/logo.c \
	cli/img.c \
	api/media.c \
	api/oauth.c \
	../lib/progress_bar.c \
	../lib/surl_find_line.c \
	../lib/surl_ping.c \
	../lib/path_helper.c

mastodon_CC65SOURCES := \
	../lib/dputs.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/fastirq$(suffix).s \
	../lib/runtime_once_clean.s

mastocli_SOURCES := \
	$(common_SOURCES) \
	cli/header.c \
	cli/print.c \
	cli/tl.c\
	api/api.c \
	api/account.c \
	api/notification.c \
	api/status.c \
	../lib/path_helper.c

mastocli_CC65SOURCES := \
	../lib/dputs.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/fastirq$(suffix).s \
	../lib/runtime_once_clean.s

mastowrite_SOURCES := \
	$(common_SOURCES) \
	cli/compose.c \
	cli/compose_header.c \
	cli/print.c \
	api/account.c \
	api/compose.c \
	api/media.c \
	api/status.c \
	../lib/file_select.c \
	../lib/progress_bar.c \
	../lib/path_helper.c

mastowrite_CC65SOURCES := \
	../lib/dputs.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/fastirq$(suffix).s \

mastoconf_SOURCES := \
	cli/config.c \
	cli/logo.c \
	../lib/clrzone.c \
	../lib/scrollwindow.c
mastoconf_CC65SOURCES :=

GCCSOURCES := ../lib/extended_conio.c ../lib/clrzone.c ../lib/extended_string.c ../lib/tgi_sdl.c ../lib/tgi_fastline.c
DEPS :=

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -I api -I cli -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x400 -DSURL_TO_LANGCARD -C ../../config/apple2enh-rtonce.cfg $(debug_CFLAGS)  $(iigs_CFLAGS)
#Using start address 0x4000 is easier than HGR page 2, and also allows to use 80 columns
mastodon_CLCFLAGS = $(CLCFLAGS) --start-addr 0x4000

GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: mastodon.bin mastodon.x86_64 \
	mastocli.bin mastocli.x86_64 \
	mastowrite.bin mastowrite.x86_64 \
	mastoconf.bin mastoconf.x86_64

mastodon.bin: $(mastodon_SOURCES) $(mastodon_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastodon.dbg" $(CFLAGS) $(mastodon_CLCFLAGS) -vm -m mastodon.map -o $@ $^
mastocli.bin: $(mastocli_SOURCES) $(mastocli_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastocli.dbg" $(CFLAGS) $(CLCFLAGS) -vm -m mastocli.map -o $@ $^
mastowrite.bin: $(mastowrite_SOURCES) $(mastowrite_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastowrite.dbg" $(CFLAGS) $(CLCFLAGS) -vm -m mastowrite.map -o $@ $^
mastoconf.bin: $(mastoconf_SOURCES) $(mastoconf_CC65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -vm -m mastoconf.map -o $@ $^

mastodon.x86_64: $(mastodon_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastocli.x86_64: $(mastocli_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastowrite.x86_64: $(mastowrite_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastoconf.x86_64: $(mastoconf_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f *.bin *.x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/mastodon/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/mastodon/ && \
	cp masto*.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/mastodon/
