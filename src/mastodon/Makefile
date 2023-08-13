mastodon_SOURCES := cli/login.c api/oauth.c cli/logo.c \
	cli/img.c api/common.c api/media.c \
	 ../lib/clrzone.c ../lib/scrollwindow.c ../lib/dgets.c \
	../lib/simple_serial.c ../lib/strsplit.c \
	../lib/surl_core.c ../lib/surl_send.c ../lib/surl_find_line.c \
	../lib/surl_get_json.c ../lib/progress_bar.c
mastodon_CC65SOURCES := ../lib/dputs.s ../lib/dputc.s ../lib/scroll.s

mastocli_SOURCES := cli/tl.c api/api.c \
	api/account.c api/status.c api/common.c \
	api/notification.c \
	cli/header.c cli/print.c \
	../lib/clrzone.c ../lib/scrollwindow.c ../lib/dgets.c \
	../lib/simple_serial.c ../lib/strsplit.c ../lib/path_helper.c \
	../lib/surl_core.c ../lib/surl_send.c ../lib/surl_get_json.c
mastocli_CC65SOURCES := ../lib/dputs.s ../lib/dputc.s ../lib/scroll.s

mastowrite_SOURCES := cli/compose.c cli/compose_header.c \
	api/compose.c api/account.c api/status.c api/common.c api/media.c \
	../lib/dgets.c ../lib/clrzone.c ../lib/scrollwindow.c \
	cli/print.c \
	../lib/simple_serial.c ../lib/strsplit.c \
	../lib/surl_core.c ../lib/surl_send.c ../lib/surl_get_json.c \
	 ../lib/file_select.c ../lib/progress_bar.c ../lib/path_helper.c
mastowrite_CC65SOURCES := ../lib/dputs.s ../lib/dputc.s ../lib/scroll.s

mastoconf_SOURCES := cli/config.c cli/logo.c ../lib/clrzone.c ../lib/scrollwindow.c
mastoconf_CC65SOURCES :=

GCCSOURCES := ../lib/extended_conio.c ../lib/clrzone.c ../lib/extended_string.c ../lib/tgi_sdl.c ../lib/tgi_fastline.c
DEPS :=

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -I api -I cli -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x0400 -DSERIAL_TO_LANGCARD -C ../../config/apple2enh-noexit.cfg
#Using start address 0x4000 is easier than HGR page 2, and also allows to use 80 columns
mastodon_CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x0400 --start-addr 0x4000  -DSERIAL_TO_LANGCARD -C ../../config/apple2enh-noexit.cfg
GCCCFLAGS = -g -O0 -Wall $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: mastodon.bin mastodon.x86_64 \
	mastocli.bin mastocli.x86_64 \
	mastowrite.bin mastowrite.x86_64 \
	mastoconf.bin mastoconf.x86_64

mastodon.bin: $(mastodon_SOURCES) $(mastodon_CC65SOURCES)
	cl65 $(CFLAGS) $(mastodon_CLCFLAGS) -vm -m mastodon.map -o $@ $^
mastocli.bin: $(mastocli_SOURCES) $(mastocli_CC65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"mastocli.dbg" $(CFLAGS) $(CLCFLAGS) -vm -m mastocli.map -o $@ $^ -Ln mastocli.lbl
mastowrite.bin: $(mastowrite_SOURCES) $(mastowrite_CC65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -vm -m mastowrite.map -o $@ $^
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
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/mastodon/ || \
	mkdir /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/mastodon/ && \
	cp masto*.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/mastodon/
