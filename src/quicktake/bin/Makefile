TARGET := apple2enh
USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

ifdef DEBUG_HD
debug_CFLAGS := $(debug_CFLAGS) -DDEBUG_HD=$(DEBUG_HD)
endif

ifdef DEBUG_FLOPPY
debug_CFLAGS := $(debug_CFLAGS) -DDEBUG_FLOPPY=$(DEBUG_FLOPPY)
endif

slowtake_SOURCES := \
	$(SRC_DIR)/qt-menu.c \
	$(SRC_DIR)/qt-serial.c \
	$(SRC_DIR)/qt1x0-serial.c \
	$(SRC_DIR)/qt200-serial.c \
	$(SRC_DIR)/qt-edit-image.c \
	$(SRC_DIR)/qt-state.c \
	$(SRC_DIR)/../lib/dget_text_single.c \
	$(SRC_DIR)/../lib/strtrim.c \
	$(SRC_DIR)/../lib/progress_bar.c \
	$(SRC_DIR)/../lib/hgr_addrs.c

slowtake_CC65SOURCES := \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_read.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_configure.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/asm/file_select.s \
	$(SRC_DIR)/../lib/dputc.s \
	$(SRC_DIR)/../lib/scroll.s \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/platform_sleep.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/runtime_once_clean.s \
	$(serial_hw_SOURCES)

slowtake_GCCSOURCES := \
	$(SRC_DIR)/../lib/serial/c/simple_serial.c \
	$(SRC_DIR)/../lib/serial/c/simple_serial_io.c \
	$(SRC_DIR)/../lib/serial/c/serial_control.c \
	$(SRC_DIR)/../lib/c/scrollwindow.c \
	$(SRC_DIR)/../lib/c/malloc0.c \
	$(SRC_DIR)/../lib/c/hgr.c \
	$(SRC_DIR)/../lib/c/file_select.c \
	$(SRC_DIR)/../lib/tgi_sdl.c \
	$(SRC_DIR)/../lib/tgi_fastline.c

qtktconv_SOURCES := \
	$(SRC_DIR)/qt-conv.c \
	$(SRC_DIR)/../lib/progress_bar.c

qtktconv_CC65SOURCES := \
	$(SRC_DIR)/qtkt.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/check_floppy.s

qtktconv_GCCSOURCES := \
	$(SRC_DIR)/qtkt.c

qtknconv_SOURCES := \
	$(SRC_DIR)/qt-conv.c \
	$(SRC_DIR)/qtkn.c \
	$(SRC_DIR)/../lib/progress_bar.c

qtknconv_CC65SOURCES := \
	$(SRC_DIR)/qtk_bithuff.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/mult16x16x32.s \
	$(SRC_DIR)/../lib/approxdiv16x8.s \
	$(SRC_DIR)/../lib/square_table.s \
	$(SRC_DIR)/../lib/check_floppy.s
	
qtknconv_GCCSOURCES := \
	$(SRC_DIR)/qtk_bithuff.c

jpegconv_SOURCES := \
	$(SRC_DIR)/qt-conv.c \
	$(SRC_DIR)/jpeg.c \
	$(SRC_DIR)/../lib/progress_bar.c

jpegconv_CC65SOURCES := \
	$(SRC_DIR)/jpeg_platform.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/check_floppy.s

jpegconv_GCCSOURCES := \
	$(SRC_DIR)/jpeg_platform.c

GCCSOURCES := \
	$(SRC_DIR)/../lib/extended_conio.c \
	$(SRC_DIR)/../lib/c/path_helper.c \
	$(SRC_DIR)/../lib/c/clrzone.c

DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I $(SRC_DIR)/../lib -I . -DCONF_FILE_PATH=\"/etc/a2tools/quicktake.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(USE_LANGCARD) $(debug_CFLAGS) $(iigs_CFLAGS)

slowtake_CLCFLAGS = $(CLCFLAGS) -C $(SRC_DIR)/../../config/apple2enh-hgr.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0xC00 #One file buffer

qtktconv_CLCFLAGS = -D_CACHE_SIZE=16384 --asm-define CACHE_SIZE=16384 \
	-DRAW_X_OFFSET=2 -DRAW_Y_OFFSET=2 -DRAW_WIDTH=645 \
	$(CLCFLAGS) -C $(SRC_DIR)/../../config/apple2enh-alignbssrodata.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000    #Two file buffers

qtknconv_CLCFLAGS = -D_CACHE_SIZE=4096 --asm-define CACHE_SIZE=4096 \
	-DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=width \
	$(CLCFLAGS) -C $(SRC_DIR)/../../config/apple2enh-alignbssdata.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000

jpegconv_CLCFLAGS = -D_CACHE_SIZE=4096 --asm-define CACHE_SIZE=4096 \
	-DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=width \
	$(CLCFLAGS) -C $(SRC_DIR)/../../config/apple2enh.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000

GCCCFLAGS = -D_CACHE_SIZE=4096 -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)
qtktconv_GCCCFLAGS = -DRAW_X_OFFSET=2 -DRAW_Y_OFFSET=2 -DRAW_WIDTH=645

qtknconv_GCCCFLAGS = -DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=width

jpegconv_GCCCFLAGS = -DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=width

GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: quicktake-$(suffix).po

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
quicktake-$(suffix).po: splash.h slowtake.bin qtktconv.bin qtknconv.bin jpegconv.bin loader.x86_64 slowtake.x86_64 qtktconv.x86_64 qtknconv.x86_64 jpegconv.x86_64 $(SRC_DIR)/../image-viewer/bin/$(suffix)/imgview.bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ QUICKTAKE
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ SLOWTAKE.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ ABOUT BIN 0x2000 < $(SRC_DIR)/about; \
	for r in *.bin $(SRC_DIR)/../image-viewer/bin/$(suffix)/imgview.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

splash.hgr: $(SRC_DIR)/splash.png
	$(SRC_DIR)/../surl-server/hgr-convert $<
	mv $<.hgr .

splash.h: splash.hgr
	$(SRC_DIR)/../../bin/make-splash.sh splash.png

slowtake.bin: $(slowtake_SOURCES) $(slowtake_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"slowtake.dbg" $(CFLAGS) $(slowtake_CLCFLAGS) -vm -m slowtake.map -o $@ $^

slowtake.x86_64: $(slowtake_SOURCES) $(slowtake_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

qtktconv.bin: $(qtktconv_CC65SOURCES) $(qtktconv_SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qtktconv.dbg" $(CFLAGS) $(qtktconv_CLCFLAGS) -vm -m qtktconv.map -o $@ $^

qtktconv.x86_64: $(qtktconv_SOURCES) $(qtktconv_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $(qtktconv_GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

qtknconv.bin: $(qtknconv_SOURCES) $(qtknconv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qtknconv.dbg" $(CFLAGS) $(qtknconv_CLCFLAGS) -vm -m qtknconv.map -o $@ $^

qtknconv.x86_64: $(qtknconv_SOURCES) $(qtknconv_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $(qtknconv_GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

jpegconv.bin: $(jpegconv_CC65SOURCES) $(jpegconv_SOURCES)
	cl65 --debug-info -Wl --dbgfile,"jpegconv.dbg" $(CFLAGS) $(jpegconv_CLCFLAGS) -vm -m jpegconv.map -o $@ $^

jpegconv.x86_64: $(jpegconv_SOURCES) $(jpegconv_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $(jpegconv_GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

loader.x86_64: $(SRC_DIR)/loader.c
	gcc -o $@ -I/usr/include/SDL $< $(SRC_DIR)/../lib/extended_conio.c $(SRC_DIR)/../lib/c/clrzone.c -lSDL_image -lSDL

clean:
	rm -f *.bin *.x86_64 *.o *.map splash.h splash.hgr *.dbg *.lbl splash.h *.hgr *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/$(suffix)/ && \
	cp *.bin $(SRC_DIR)/about /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/$(suffix)/

dist: quicktake-$(suffix).po
	mkdir -p $(SRC_DIR)../../dist/
	cp $< $(SRC_DIR)../../dist/
