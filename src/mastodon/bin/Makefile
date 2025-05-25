GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	-DDYN_BOX_DRAW

common_CC65SOURCES := \
	$(SRC_DIR)/../lib/serial/asm/driver.s \
	$(SRC_DIR)/../lib/serial/asm/acia.s \
	$(SRC_DIR)/../lib/serial/asm/z8530.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_get_json.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/dputs.s \
	$(SRC_DIR)/../lib/dputc.s \
	$(SRC_DIR)/../lib/scroll.s \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/a2_features.s \
	$(SRC_DIR)/../lib/oa_cgetc.s \
	$(SRC_DIR)/cli/initconst.s \
	$(SRC_DIR)/api/asm/common.s

mastodon_SOURCES := \
	$(SRC_DIR)/api/oauth.c \
	$(SRC_DIR)/cli/login.c \
	$(SRC_DIR)/cli/logo.c \
	$(SRC_DIR)/cli/config.c \
	$(SRC_DIR)/../lib/dget_text_single.c \
	$(SRC_DIR)/../lib/strsplit.c \
	$(SRC_DIR)/../lib/charsets.c

mastodon_CC65SOURCES := \
	$(common_CC65SOURCES) \
	$(SRC_DIR)/../lib/surl/asm/surl_send.s \
	$(SRC_DIR)/../lib/surl/asm/surl_find_line.s \
	$(SRC_DIR)/../lib/surl/asm/surl_ping.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_configure.s \
	$(SRC_DIR)/../lib/runtime_once_clean.s \
	$(SRC_DIR)/../lib/citoa.s \
	$(SRC_DIR)/../lib/cutoa.s \
	$(SRC_DIR)/../lib/cntoabuf.s

mastoimg_SOURCES := \
	$(SRC_DIR)/api/media.c \
	$(SRC_DIR)/cli/img.c \
	$(SRC_DIR)/../lib/strsplit.c \
	$(SRC_DIR)/../lib/dget_text_single.c

mastoimg_CC65SOURCES := \
	$(SRC_DIR)/../lib/surl/surl_stream_av/$(CPU).s \
	$(common_CC65SOURCES) \
	$(SRC_DIR)/../lib/a2_features.s \
	$(SRC_DIR)/../lib/asm/hgr_mono40.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/asm/file_select.s \
	$(SRC_DIR)/../lib/asm/hyphenize.s \
	$(SRC_DIR)/../lib/vsdrive.s

mastocli_SOURCES := \
	$(SRC_DIR)/api/api.c \
	$(SRC_DIR)/api/account.c \
	$(SRC_DIR)/api/account_new_from_json.c \
	$(SRC_DIR)/api/notification.c \
	$(SRC_DIR)/api/status.c \
	$(SRC_DIR)/api/poll.c \
	$(SRC_DIR)/api/poll_update_vote.c \
	$(SRC_DIR)/api/date_format.c \
	$(SRC_DIR)/cli/header.c \
	$(SRC_DIR)/cli/print.c \
	$(SRC_DIR)/cli/tl.c\
	$(SRC_DIR)/../lib/dget_text_single.c \
	$(SRC_DIR)/../lib/strsplit.c \
	$(SRC_DIR)/../lib/progress_bar.c

mastocli_CC65SOURCES := \
	$(common_CC65SOURCES) \
	$(SRC_DIR)/../lib/surl/asm/surl_send.s \
	$(SRC_DIR)/../lib/asm/atoc.s \
	$(SRC_DIR)/../lib/runtime_once_clean.s

mastowrite_SOURCES := \
	$(SRC_DIR)/api/account.c \
	$(SRC_DIR)/api/compose.c \
	$(SRC_DIR)/api/media.c \
	$(SRC_DIR)/api/status.c \
	$(SRC_DIR)/api/poll.c \
	$(SRC_DIR)/api/date_format.c \
	$(SRC_DIR)/cli/compose.c \
	$(SRC_DIR)/cli/compose_header.c \
	$(SRC_DIR)/cli/print.c \
	$(SRC_DIR)/../lib/dget_text_multi.c \
	$(SRC_DIR)/../lib/strsplit.c \
	$(SRC_DIR)/../lib/progress_bar.c \

mastowrite_CC65SOURCES := \
	$(common_CC65SOURCES) \
	$(SRC_DIR)/../lib/surl/asm/surl_send.s \
	$(SRC_DIR)/../lib/asm/atoc.s \
	$(SRC_DIR)/../lib/asm/file_select.s \
	$(SRC_DIR)/../lib/asm/hyphenize.s \
	$(SRC_DIR)/../lib/runtime_once_clean.s \
	$(SRC_DIR)/../lib/vsdrive.s

GCCSOURCES := \
	$(SRC_DIR)/api/c/common.c \
	$(SRC_DIR)/../lib/serial/c/simple_serial.c \
	$(SRC_DIR)/../lib/serial/c/simple_serial_io.c \
	$(SRC_DIR)/../lib/surl/c/surl_core.c \
	$(SRC_DIR)/../lib/surl/c/surl_ping.c \
	$(SRC_DIR)/../lib/surl/c/surl_find_line.c \
	$(SRC_DIR)/../lib/surl/c/surl_send.c \
	$(SRC_DIR)/../lib/surl/c/surl_get_json.c \
	$(SRC_DIR)/../lib/c/scrollwindow.c \
	$(SRC_DIR)/../lib/c/clrzone.c \
	$(SRC_DIR)/../lib/c/path_helper.c \
	$(SRC_DIR)/../lib/c/malloc0.c \
	$(SRC_DIR)/../lib/c/hgr.c \
	$(SRC_DIR)/../lib/c/atoc.c \
	$(SRC_DIR)/../lib/c/file_select.c \
	$(SRC_DIR)/../lib/c/hyphenize.c \
	$(SRC_DIR)/../lib/extended_conio.c \
	$(SRC_DIR)/../lib/strtrim.c \
	$(SRC_DIR)/../lib/tgi_sdl.c \
	$(SRC_DIR)/../lib/tgi_fastline.c

DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I $(SRC_DIR)/../lib -I $(SRC_DIR)/api -I $(SRC_DIR)/cli -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x400 $(GENERAL_CFLAGS) $(debug_CFLAGS) \

mastodon_CLCFLAGS = $(CLCFLAGS) -C $(SRC_DIR)/../../config/apple2enh-rtonce.cfg --start-addr 0xC03 #One file buffer
mastocli_CLCFLAGS = $(CLCFLAGS) -C $(SRC_DIR)/../../config/apple2enh-rtonce.cfg --start-addr 0xC03 #One file buffer
mastoimg_CLCFLAGS = $(CLCFLAGS) -C $(SRC_DIR)/../../config/apple2enh-mastoimg-6502.cfg \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-DSTART_ADDR=0x5DA0 --start-addr 0x5DA0
mastowrite_CLCFLAGS = $(CLCFLAGS) -C $(SRC_DIR)/../../config/apple2enh-rtonce.cfg --start-addr 0xC03 #One file buffer

GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all dist: mastodon-$(suffix).po

perso: all mastoperso-$(suffix).po

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
mastodon-$(suffix).po: mastodon.bin mastocli.bin mastowrite.bin mastoimg.bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ MASTODON
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ MASTODON.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ IMGLOW BIN < IMGLOW; \
	for r in *.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

mastoperso-$(suffix).po: mastodon-$(suffix).po
	cp $< $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ mastsettings TXT < $(SRC_DIR)/mastsettings; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ clisettings TXT < $(SRC_DIR)/clisettings; \

mastodon.bin: $(mastodon_SOURCES) $(mastodon_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastodon.dbg" $(CFLAGS) $(mastodon_CLCFLAGS) -vm -m mastodon.map -o $@ $^ $(CC65_TARGET)-iobuf-0800.o
mastocli.bin: $(mastocli_SOURCES) $(mastocli_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastocli.dbg" $(CFLAGS) $(mastocli_CLCFLAGS) -vm -m mastocli.map -o $@ $^ $(CC65_TARGET)-iobuf-0800.o
mastowrite.bin: $(mastowrite_SOURCES) $(mastowrite_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"mastowrite.dbg" $(CFLAGS) $(mastowrite_CLCFLAGS) -vm -m mastowrite.map -o $@ $^ $(CC65_TARGET)-iobuf-0800.o
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
	rm -f *.bin *.x86_64 *.o *.map *.po *.dbg

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/mastodon/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/mastodon/$(suffix)/ && \
	cp masto*.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/mastodon/$(suffix)/


dist: mastodon-$(suffix).po
	mkdir -p $(SRC_DIR)../../dist/
	cp $< $(SRC_DIR)../../dist/
