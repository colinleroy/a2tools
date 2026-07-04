TARGET := apple2
CPU := 6502

FLOP_NAME = SIXFORTY
CLEANDISK = ../../disks/ProDOS_2_4_3.po

GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	-DAPPLE2_INCLUDE_IIE_CHARS \

START_ADDR := 0x1100

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -I ../stp -I . -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror -DSTP_DATA_SIZE=8192

.PHONY: all clean

sixforty_SOURCES := \
	main.c \
	api.c \
	comment.c \
	post.c \
	../lib/scroll.s \
	../lib/dputc.s \
	../lib/dputs.s \
	../lib/platform_sleep.s \
	../lib/FVTABZ.s \
	../lib/dget_text_multi.c \
	../lib/progress_bar.c \
	../lib/strsplit.c \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_send.s \
	../lib/surl/asm/surl_get_json.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/surl/asm/surl_read_image.s \
	../lib/asm/file_select.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/asm/hgr.s \
	../lib/asm/backup_hgrpage.s \
	../lib/asm/reserve_auxhgr.s \
	../lib/asm/unlink_auxhgr.s \
	../lib/asm/auxhgr_file.s \
	../lib/asm/hgr_setup_mono40.s \
	../lib/asm/hgr_mono40.s \
	../lib/surl/asm/surl_ping.s \
	../lib/vsdrive.s \
	../lib/extrazp.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s \
	../lib/a2_features.s \
	../lib/charsets.c \

sixforty_CLCFLAGS = -t $(TARGET) -I ../lib -I . \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-sixforty.cfg \
	-vm -m sixforty.map $(GENERAL_CFLAGS) -O

all: sixforty.po

add_disk:
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)SIXFORTY.SYSTEM SYS < ../../bin/loader.system; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $(IMG) $(PREFIX)$$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

sixforty.po: splash.h sixforty.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ $(FLOP_NAME)
	IMG=$@ PREFIX= make add_disk

hdv: 
	../../bin/cp2/cp2 mkdir $(IMG) $(FLOP_NAME)
	../../bin/cp2/cp2 copy --recurse sixforty.po $(IMG):$(FLOP_NAME)

sixfortyperso.po: sixforty.po
	cp $< $@; \
	java -jar ../../bin/ac.jar -p $@ SFSETTINGS TXT < SFSETTINGS; \


clean:
	rm -f *.bin *.o *.map splash*.h *.hgr *.po *.dbg *.lbl

sixforty.hgr: sixforty.png
	../surl-server/hgr-convert $<

splash.h: sixforty.hgr
	../../bin/make-splash.sh sixforty.png splash.h

sixforty.bin: $(sixforty_CL65SOURCES) $(sixforty_SOURCES)
	cl65 -g -Wl --dbgfile,sixforty.dbg -Ln sixforty.lbl $(CFLAGS) $(sixforty_CLCFLAGS) -o $@ $^  $(TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/sixforty/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/sixforty/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/sixforty/

dist: sixforty.po
	mkdir -p ../../dist/
	cp $< ../../dist/
