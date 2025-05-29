TARGET := apple2

GENERAL_CFLAGS := -DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	-DDYN_BOX_DRAW \

imgview_SOURCES := \
	imageview.c \
	../lib/progress_bar.c \
	../lib/hgr_addrs.c \

imgview_CC65SOURCES := \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/serial/asm/simple_serial_getc_immediate.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/asm/hgr.s \
	../lib/asm/file_select.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/extrazp.s \
	../lib/vsdrive.s \
	../lib/a2_features.s \

imgview_GCCSOURCES := \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/serial/c/serial_control.c \
	../lib/c/scrollwindow.c \
	../lib/c/path_helper.c \
	../lib/c/clrzone.c \
	../lib/c/malloc0.c \
	../lib/c/hgr.c \
	../lib/c/file_select.c \
	../lib/tgi_sdl.c \
	../lib/tgi_fastline.c \
	../lib/strtrim.c

GCCSOURCES := \
	../lib/extended_conio.c \
	../lib/c/clrzone.c

DEPS :=

CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(GENERAL_CFLAGS)

imgview_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-rtonce.cfg $(TARGET)-iobuf-0800.o --start-addr 0x4000

GCCCFLAGS = -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: imageviewer.po

CLEANDISK = ../../disks/ProDOS_2_4_3.po
imageviewer.po: imgview.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ IMGVIEW
	java -jar ../../bin/ac.jar -p $@ IMGVIEW.SYSTEM SYS < ../../bin/loader.system; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

imgview.bin: $(imgview_SOURCES) $(imgview_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"imgview.dbg" $(CFLAGS) $(imgview_CLCFLAGS) -vm -m imgview.map -o $@ $^

imgview.x86_64: $(imgview_SOURCES) $(imgview_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f *.bin *.x86_64 *.o *.map *.dbg *.lbl *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/imageviewer/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/imageviewer/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/imageviewer/
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/

dist: imageviewer.po
	mkdir -p ../../dist/
	cp $< ../../dist/
