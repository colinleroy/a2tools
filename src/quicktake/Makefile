ifdef SER_DEBUG
debug_CFLAGS := -DSER_DEBUG=1
debug_SOURCES :=
else
debug_CFLAGS :=
debug_SOURCES :=
endif

TARGET := apple2enh
USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

ifdef DEBUG_HD
debug_CFLAGS := $(debug_CFLAGS) -DDEBUG_HD=$(DEBUG_HD)
endif

ifdef DEBUG_FLOPPY
debug_CFLAGS := $(debug_CFLAGS) -DDEBUG_FLOPPY=$(DEBUG_FLOPPY)
endif

ifdef IIGS
serial_hw_SOURCES := ../lib/serial/z8530/common.s ../lib/serial/z8530/control.s ../lib/serial/z8530/no_irq.s
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs
else
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/control.s ../lib/serial/acia/no_irq.s
ifdef OLDII
iigs_FLAGS :=
suffix :=
upload_subdir :=
TARGET := apple2
USE_LANGCARD :=
else
iigs_FLAGS :=
suffix :=
upload_subdir :=
endif
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
	../lib/hgr_addrs.c \
	$(debug_SOURCES)

#Fastirq needed, quicktake don't wait on us
slowtake_CC65SOURCES := \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_read.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/malloc0.s \
	../lib/asm/hgr.s \
	../lib/asm/file_select.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/asm/clrzone.s \
	../lib/platform_sleep.s \
	../lib/extrazp.s \
	../lib/runtime_once_clean.s \
	$(serial_hw_SOURCES) \
	../lib/fastirq$(suffix).s \

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
	../lib/asm/path_helper.s \
	../lib/extrazp.s \
	../lib/check_floppy.s

qtktconv_GCCSOURCES := \
	qtkt.c

qtknconv_SOURCES := \
	qt-conv.c \
	qtkn.c \
	../lib/progress_bar.c

qtknconv_CC65SOURCES := \
	qtk_bithuff.s \
	../lib/extrazp.s \
	../lib/asm/path_helper.s \
	../lib/mult16x16x32.s \
	../lib/approxdiv16x8.s \
	../lib/square_table.s \
	../lib/check_floppy.s
	
qtknconv_GCCSOURCES := \
	qtk_bithuff.c

jpegconv_SOURCES := \
	qt-conv.c \
	jpeg.c \
	../lib/progress_bar.c

jpegconv_CC65SOURCES := \
	jpeg_platform.s \
	../lib/asm/path_helper.s \
	../lib/extrazp.s \
	../lib/check_floppy.s

jpegconv_GCCSOURCES := \
	jpeg_platform.c

GCCSOURCES := \
	../lib/extended_conio.c \
	../lib/c/path_helper.c \
	../lib/c/clrzone.c

DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/quicktake.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(USE_LANGCARD) $(debug_CFLAGS) $(iigs_CFLAGS)

slowtake_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-hgr.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0xC00 #One file buffer

qtktconv_CLCFLAGS = -D_CACHE_SIZE=16384 --asm-define CACHE_SIZE=16384 \
	-DRAW_X_OFFSET=2 -DRAW_Y_OFFSET=2 -DRAW_WIDTH=645 \
	$(CLCFLAGS) -C ../../config/apple2enh-alignbssrodata.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000    #Two file buffers

qtknconv_CLCFLAGS = -D_CACHE_SIZE=4096 --asm-define CACHE_SIZE=4096 \
	-DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=width \
	$(CLCFLAGS) -C ../../config/apple2enh-alignbssdata.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000

jpegconv_CLCFLAGS = -D_CACHE_SIZE=4096 --asm-define CACHE_SIZE=4096 \
	-DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=width \
	$(CLCFLAGS) -C ../../config/apple2enh.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000

GCCCFLAGS = -D_CACHE_SIZE=4096 -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)
qtktconv_GCCCFLAGS = -DRAW_X_OFFSET=2 -DRAW_Y_OFFSET=2 -DRAW_WIDTH=645

qtknconv_GCCCFLAGS = -DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=width

jpegconv_GCCCFLAGS = -DRAW_X_OFFSET=0 -DRAW_Y_OFFSET=0 -DRAW_WIDTH=width

GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: \
	splash.h \
	slowtake.bin slowtake.x86_64 \
	qtktconv.bin qtktconv.x86_64 \
	qtknconv.bin qtknconv.x86_64 \
	jpegconv.bin jpegconv.x86_64 \
	loader.x86_64

splash.hgr:
	../surl-server/hgr-convert splash.png

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
	gcc -o $@ -I/usr/include/SDL loader.c ../lib/extended_conio.c ../lib/c/clrzone.c -lSDL_image -lSDL

clean:
	rm -f *.bin *.x86_64 *.o *.map splash.h splash.hgr

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ && \
	cp *.bin about /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/
