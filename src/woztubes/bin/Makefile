USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

START_ADDR := 0x113A

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I $(SRC_DIR)/../lib -I $(SRC_DIR)/../stp -I . -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror -DSTP_DATA_SIZE=8192

.PHONY: all clean

woztubes_SOURCES := \
	$(SRC_DIR)/../lib/surl/surl_stream_av/$(STREAMER_VER).s \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/video_providers.c \
	$(SRC_DIR)/config.c \
	$(SRC_DIR)/../lib/scroll.s \
	$(SRC_DIR)/../lib/dputc.s \
	$(SRC_DIR)/../lib/dget_text_single.c \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/strsplit.c \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_configure.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/common.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/no_irq.s \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_get_json.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/surl/asm/surl_ping.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/citoa.s \
	$(SRC_DIR)/../lib/cutoa.s \
	$(SRC_DIR)/../lib/cntoabuf.s \
	$(SRC_DIR)/../lib/charsets.c

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

woztubes_CLCFLAGS = -t $(CC65_TARGET) -I $(SRC_DIR)/../lib -I . \
	-DBUF_1K_ADDR=0x800 -DBUF_1K_SIZE=0x3FF \
	-DBUF_8K_ADDR=0x4000 -DBUF_8K_SIZE=0x1FFF \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-Wl -D,__STACKSIZE__=0x0400 -C $(SRC_DIR)/../../config/apple2enh-woztubes.cfg \
	-vm -m woztubes.map $(USE_LANGCARD) -O --start-addr $(START_ADDR)

all: woztubes-$(suffix).po

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
woztubes-$(suffix).po: splash.h woztubes.bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ WOZTUBES
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ WOZTUBES.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	for r in *.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

woztubesperso-$(suffix).po: woztubes-$(suffix).po
	cp $< $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ STPSTARTURL TXT < $(SRC_DIR)/STPSTARTURL; \


clean:
	rm -f *.bin *.o *.map splash*.h *.hgr *.po *.dbg *.lbl

woztubes.hgr: $(SRC_DIR)/woztubes.png
	$(SRC_DIR)../surl-server/hgr-convert $<
	mv $<.hgr .

splash.h: woztubes.hgr
	$(SRC_DIR)../../bin/make-splash.sh woztubes.png splash.h

woztubes.bin: $(woztubes_CL65SOURCES) $(woztubes_SOURCES)
	cl65 -g -Wl --dbgfile,woztubes.dbg -Ln woztubes.lbl $(CFLAGS) $(iigs_CFLAGS) $(woztubes_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/woztubes/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/woztubes/$(suffix)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/woztubes/$(suffix)/

dist: woztubes-$(suffix).po
	mkdir -p $(SRC_DIR)../../dist/
	cp $< $(SRC_DIR)../../dist/
