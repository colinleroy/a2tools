GENERAL_CFLAGS := -DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ

imgview_SOURCES := \
	$(SRC_DIR)/imageview.c \
	$(SRC_DIR)/../lib/progress_bar.c \
	$(SRC_DIR)/../lib/hgr_addrs.c \

imgview_CC65SOURCES := \
	$(SRC_DIR)/../lib/serial/asm/driver.s \
	$(SRC_DIR)/../lib/serial/asm/acia.s \
	$(SRC_DIR)/../lib/serial/asm/z8530.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_configure.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_getc_immediate.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/asm/file_select.s \
	$(SRC_DIR)/../lib/dputc.s \
	$(SRC_DIR)/../lib/scroll.s \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/vsdrive.s \
	$(SRC_DIR)/../lib/a2_features.s \

imgview_GCCSOURCES := \
	$(SRC_DIR)/../lib/serial/c/simple_serial.c \
	$(SRC_DIR)/../lib/serial/c/simple_serial_io.c \
	$(SRC_DIR)/../lib/serial/c/serial_control.c \
	$(SRC_DIR)/../lib/c/scrollwindow.c \
	$(SRC_DIR)/../lib/c/path_helper.c \
	$(SRC_DIR)/../lib/c/clrzone.c \
	$(SRC_DIR)/../lib/c/malloc0.c \
	$(SRC_DIR)/../lib/c/hgr.c \
	$(SRC_DIR)/../lib/c/file_select.c \
	$(SRC_DIR)/../lib/tgi_sdl.c \
	$(SRC_DIR)/../lib/tgi_fastline.c \
	$(SRC_DIR)/../lib/strtrim.c

GCCSOURCES := \
	$(SRC_DIR)/../lib/extended_conio.c \
	$(SRC_DIR)/../lib/c/clrzone.c

DEPS :=

CFLAGS  = -I $(SRC_DIR)/../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(GENERAL_CFLAGS)

imgview_CLCFLAGS = $(CLCFLAGS) -C $(SRC_DIR)/../../config/apple2enh-rtonce.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x4000

GCCCFLAGS = -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: imageviewer-$(suffix).po

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
imageviewer-$(suffix).po: imgview.bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ IMGVIEW
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ IMGVIEW.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	for r in *.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

imgview.bin: $(imgview_SOURCES) $(imgview_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"imgview.dbg" $(CFLAGS) $(imgview_CLCFLAGS) -vm -m imgview.map -o $@ $^

imgview.x86_64: $(imgview_SOURCES) $(imgview_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f *.bin *.x86_64 *.o *.map *.dbg *.lbl *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/imageviewer/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/imageviewer/$(suffix)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/imageviewer/$(suffix)/
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/$(suffix)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/$(suffix)/

dist: imageviewer-$(suffix).po
	mkdir -p $(SRC_DIR)../../dist/
	cp $< $(SRC_DIR)../../dist/
