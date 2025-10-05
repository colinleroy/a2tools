TARGET := apple2

GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	--asm-define SERIAL_LOW_LEVEL_CONTROL \
	-DDYN_BOX_DRAW -DAPPLE2_INCLUDE_IIE_CHARS \

QTKN_DIVISION := APPROX_DIVISION
#QTKN_DIVISION := PRECISE_DIVISION

ifdef DEBUG_HD
debug_CFLAGS := $(debug_CFLAGS) -DDEBUG_HD=$(DEBUG_HD)
endif

ifdef DEBUG_FLOPPY
debug_CFLAGS := $(debug_CFLAGS) -DDEBUG_FLOPPY=$(DEBUG_FLOPPY)
endif

slowtake_SOURCES := \
	qt-menu.c \
	qt-serial.c \
	qt1x0-serial.c \
	qt200-serial.c \
	qt-edit-image.c \
	qt-state.c \
	../lib/dget_text_single.c \
	../lib/strtrim.c \
	../lib/progress_bar.c \
	../lib/hgr_addrs.c

slowtake_CC65SOURCES := \
	../lib/hgr_addrs.s \
	qt-dither-horiz.s \
	qt-dither-vert.s \
	qt-dither-common.s \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_read.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/fastirq.s \
	../lib/fastirq-iigs.s \
	../lib/a2_features.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/malloc0.s \
	../lib/asm/hgr.s \
	../lib/asm/reserve_auxhgr.s \
	../lib/asm/auxhgr_file.s \
	../lib/asm/unlink_auxhgr.s \
	../lib/asm/backup_screen_holes.s \
	../lib/asm/file_select.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/asm/clrzone.s \
	../lib/platform_sleep.s \
	../lib/extrazp.s \
	../lib/runtime_once_clean.s \
	../lib/a2_features.s \

slowtake_GCCSOURCES := \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/serial/c/serial_control.c \
	../lib/c/scrollwindow.c \
	../lib/c/malloc0.c \
	../lib/c/hgr.c \
	../lib/c/file_select.c \
	../lib/tgi_sdl.c \
	../lib/tgi_fastline.c

qtktconv_SOURCES := \
	qt-conv.c \
	../lib/progress_bar.c

qtktconv_CC65SOURCES := \
	qtkt.s \
	../lib/asm/reserve_auxhgr.s \
	../lib/asm/auxhgr_file.s \
	../lib/asm/unlink_auxhgr.s \
	../lib/asm/path_helper.s \
	../lib/extrazp.s \
	../lib/a2_features.s \
	../lib/check_floppy.s

qtktconv_GCCSOURCES := \
	qtkt.c

qtknconv_SOURCES := \
	qt-conv.c \
	qtkn.c \
	../lib/progress_bar.c

qtknconv_CC65SOURCES := \
	qtk_bithuff.s \
	qtkn_platform.s \
	../lib/asm/reserve_auxhgr.s \
	../lib/asm/auxhgr_file.s \
	../lib/asm/unlink_auxhgr.s \
	../lib/extrazp.s \
	../lib/asm/path_helper.s \
	../lib/mult8x8x16.s \
	../lib/mult16x16mid16.s \
	../lib/approxdiv16x8.s \
	../lib/square_table.s \
	../lib/a2_features.s \
	../lib/check_floppy.s
	
qtknconv_GCCSOURCES := \
	qtk_bithuff.c \
	qtkn_platform.c

jpegconv_SOURCES := \
	qt-conv.c \
	jpeg.c \
	../lib/progress_bar.c

jpegconv_CC65SOURCES := \
	jpeg_arrays.s \
	jpeg_platform.s \
	../lib/asm/reserve_auxhgr.s \
	../lib/asm/auxhgr_file.s \
	../lib/asm/unlink_auxhgr.s \
	../lib/mult16x16x16.s \
	../lib/square_table.s \
	../lib/asm/path_helper.s \
	../lib/extrazp.s \
	../lib/a2_features.s \
	../lib/check_floppy.s

jpegconv_GCCSOURCES := \
	jpeg_platform.c

GCCSOURCES := \
	../lib/extended_conio.c \
	../lib/c/path_helper.c \
	../lib/c/clrzone.c

DEPS :=

CFLAGS  = -I ../lib -I . -DCONF_FILE_PATH=\"/etc/a2tools/quicktake.conf\"

CLCFLAGS = -t $(TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(GENERAL_CFLAGS) $(debug_CFLAGS) \

slowtake_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-hgr-alignbss.cfg $(TARGET)-iobuf-0800.o \
	-DPROGRAM_NAME='"SLOWTAKE"' -DBUFFER_SIZE=2048 --asm-define BUFFER_SIZE=2048 \
	--start-addr 0xC00 #One file buffer

# 4 full tracks cache!
qtktconv_CLCFLAGS = -D_CACHE_SIZE=20480 --asm-define CACHE_SIZE=20480 \
	-DRAW_X_OFFSET=1 -DRAW_Y_OFFSET=1 \
	-DRAW_WIDTH=768 --asm-define RAW_WIDTH=768 \
	-DQTKTCONV=1 \
	$(CLCFLAGS) -C ../../config/apple2enh-alignbssrodata.cfg $(TARGET)-iobuf-0800.o --start-addr 0x1000    #Two file buffers

# 3.5 tracks cache
qtknconv_CLCFLAGS = -D_CACHE_SIZE=16384 --asm-define CACHE_SIZE=16384 \
	-DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=320 \
	-DQTKNCONV=1 \
	$(CLCFLAGS) -C ../../config/apple2enh-alignbssdata.cfg $(TARGET)-iobuf-0800.o --start-addr 0x1000 \
	--asm-define $(QTKN_DIVISION)=1 \

jpegconv_CLCFLAGS = -D_CACHE_SIZE=16384 --asm-define CACHE_SIZE=16384 \
	-DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=320 \
	-DJPEGCONV=1 \
	$(CLCFLAGS) -C ../../config/apple2enh-alignbssdata.cfg $(TARGET)-iobuf-0800.o --start-addr 0x1000

GCCCFLAGS = -g -Wall -Wno-pointer-sign -Wno-char-subscripts \
	-Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas \
	-fno-omit-frame-pointer \
	$(shell sdl-config --cflags)
qtktconv_GCCCFLAGS = -D_CACHE_SIZE=19200 -DRAW_X_OFFSET=1 -DRAW_Y_OFFSET=1 -DRAW_WIDTH=768 \
	-DFINAL_WIDTH=640 -DQTKTCONV=1

qtknconv_GCCCFLAGS = -D_CACHE_SIZE=14336 -DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=320 \
	-DFINAL_WIDTH=320 -DQTKNCONV=1 \
	-D$(QTKN_DIVISION)=1

jpegconv_GCCCFLAGS = -D_CACHE_SIZE=14336 -DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=320 \
	-DFINAL_WIDTH=320 -DJPEGCONV=1

GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: quicktake.po

../image-viewer/imgview.bin:
	make -C ../image-viewer/ clean all

CLEANDISK = ../../disks/ProDOS_2_4_3.po
quicktake.po: splash.h slowtake.bin qtktconv.bin qtknconv.bin jpegconv.bin loader.x86_64 qtktconv.x86_64 qtknconv.x86_64 jpegconv.x86_64 ../image-viewer/imgview.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ QUICKTAKE
	java -jar ../../bin/ac.jar -p $@ SLOWTAKE.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar ../../bin/ac.jar -p $@ ABOUT BIN 0x2000 < about; \
	for r in *.bin ../image-viewer/imgview.bin; do \
		java -jar ../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

splash.hgr: splash.png
	../surl-server/hgr-convert $<

splash.h: splash.hgr
	../../bin/make-splash.sh splash.png

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

loader.x86_64: loader.c
	gcc -o $@ -I/usr/include/SDL $< ../lib/extended_conio.c ../lib/c/clrzone.c -lSDL_image -lSDL

clean:
	rm -f *.bin *.x86_64 *.o *.map splash.h splash.hgr *.dbg *.lbl splash.h *.hgr *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/ && \
	cp *.bin about /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/ && \
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/

dist: quicktake.po
	mkdir -p ../../dist/
	cp $< ../../dist/
