TARGET := apple2
CPU := 6502

GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	-DDYN_BOX_DRAW

common_CC65SOURCES := \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
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
	../lib/a2_features.s \
	../lib/oa_cgetc.s \
	cli/initconst.s \
	api/asm/common.s

mastodon_SOURCES := \
	api/oauth.c \
	cli/login.c \
	cli/logo.c \
	cli/config.c \
	../lib/dget_text_single.c \
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
	api/media.c \
	cli/img.c \
	../lib/strsplit.c \
	../lib/dget_text_single.c

mastoimg_CC65SOURCES := \
	../lib/surl/surl_stream_av/$(CPU).s \
	$(common_CC65SOURCES) \
	../lib/a2_features.s \
	../lib/asm/hgr_mono40.s \
	../lib/asm/hgr.s \
	../lib/asm/file_select.s \
	../lib/asm/hyphenize.s \
	../lib/vsdrive.s

mastocli_SOURCES := \
	api/api.c \
	api/account.c \
	api/account_new_from_json.c \
	api/notification.c \
	api/status.c \
	api/poll.c \
	api/poll_update_vote.c \
	api/date_format.c \
	cli/header.c \
	cli/print.c \
	cli/tl.c\
	../lib/dget_text_single.c \
	../lib/strsplit.c \
	../lib/progress_bar.c

mastocli_CC65SOURCES := \
	$(common_CC65SOURCES) \
	../lib/surl/asm/surl_send.s \
	../lib/asm/atoc.s \
	../lib/runtime_once_clean.s

mastowrite_SOURCES := \
	api/account.c \
	api/compose.c \
	api/media.c \
	api/status.c \
	api/poll.c \
	api/date_format.c \
	cli/compose.c \
	cli/compose_header.c \
	cli/print.c \
	../lib/dget_text_multi.c \
	../lib/strsplit.c \
	../lib/progress_bar.c \

mastowrite_CC65SOURCES := \
	$(common_CC65SOURCES) \
	../lib/surl/asm/surl_send.s \
	../lib/asm/atoc.s \
	../lib/asm/file_select.s \
	../lib/asm/hyphenize.s \
	../lib/runtime_once_clean.s \
	../lib/vsdrive.s

GCCSOURCES := \
	api/c/common.c \
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
	../lib/c/file_select.c \
	../lib/c/hyphenize.c \
	../lib/extended_conio.c \
	../lib/strtrim.c \
	../lib/tgi_sdl.c \
	../lib/tgi_fastline.c

DEPS :=

CFLAGS  = -I ../lib -I api -I cli -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x400 $(GENERAL_CFLAGS) $(debug_CFLAGS) \

mastodon_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-rtonce.cfg --start-addr 0xC03 #One file buffer
mastocli_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-rtonce.cfg --start-addr 0xC03 #One file buffer
mastoimg_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-mastoimg-6502.cfg \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-DSTART_ADDR=0x5DA0 --start-addr 0x5DA0
mastowrite_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-rtonce.cfg --start-addr 0xC03 #One file buffer

GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all dist: mastodon.po

perso: all mastoperso.po

CLEANDISK = ../../disks/ProDOS_2_4_3.po
mastodon.po: mastodon.bin mastocli.bin mastowrite.bin mastoimg.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ MASTODON
	java -jar ../../bin/ac.jar -p $@ MASTODON.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar ../../bin/ac.jar -p $@ IMGLOW BIN < IMGLOW; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

mastoperso.po: mastodon.po
	cp $< $@; \
	java -jar ../../bin/ac.jar -p $@ mastsettings TXT < mastsettings; \
	java -jar ../../bin/ac.jar -p $@ clisettings TXT < clisettings; \

mastodon.bin: $(mastodon_SOURCES) $(mastodon_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastodon.dbg" $(CFLAGS) $(mastodon_CLCFLAGS) -vm -m mastodon.map -o $@ $^ $(TARGET)-iobuf-0800.o
mastocli.bin: $(mastocli_SOURCES) $(mastocli_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastocli.dbg" $(CFLAGS) $(mastocli_CLCFLAGS) -vm -m mastocli.map -o $@ $^ $(TARGET)-iobuf-0800.o
mastowrite.bin: $(mastowrite_SOURCES) $(mastowrite_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastowrite.dbg" $(CFLAGS) $(mastowrite_CLCFLAGS) -vm -m mastowrite.map -o $@ $^ $(TARGET)-iobuf-0800.o
mastoimg.bin: $(mastoimg_CC65SOURCES) $(mastoimg_SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastoimg.dbg" $(CFLAGS) $(mastoimg_CLCFLAGS) -vm -m mastoimg.map -o $@ $^ $(TARGET)-iobuf-0800.o

mastodon.x86_64: $(mastodon_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastocli.x86_64: $(mastocli_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastowrite.x86_64: $(mastowrite_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastoimg.x86_64: $(mastoimg_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f *.bin *.x86_64 *.o *.map *.po *.dbg

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/mastodon/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/mastodon/ && \
	cp masto*.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/mastodon/
	cp mastodon.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/


dist: mastodon.po
	mkdir -p ../../dist/
	cp $< ../../dist/
