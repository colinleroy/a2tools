ifdef SER_DEBUG
debug_CFLAGS := -DSER_DEBUG=1
debug_SOURCES :=  ../lib/simple_serial_ext.c
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
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs
else
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
	qt-menu.c qt-serial.c qt1x0-serial.c qt200-serial.c qt-edit-image.c \
	qt-state.c \
	../lib/dgets.c ../lib/scrollwindow.c ../lib/path_helper.c \
	../lib/simple_serial.c ../lib/simple_serial_ext.c \
	../lib/strtrim.c ../lib/malloc0.c \
	../lib/file_select.c ../lib/progress_bar.c ../lib/hgr.c \
	../lib/hgr_addrs.c ../lib/scrollwindow.c \
	$(debug_SOURCES)

slowtake_CC65SOURCES := \
	../lib/dputc.s ../lib/scroll.s ../lib/FVTABZ.s \
	../lib/clrzone.s ../lib/fastirq$(suffix).s \
	../lib/platform_sleep.s ../lib/extrazp.s \
	../lib/runtime_once_clean.s \
	../lib/simple_serial_io.s

slowtake_GCCSOURCES := ../lib/tgi_sdl.c ../lib/tgi_fastline.c

qtktconv_SOURCES := \
	qt-conv.c ../lib/path_helper.c ../lib/progress_bar.c

qtktconv_CC65SOURCES := qtkt.s ../lib/extrazp.s
qtktconv_GCCSOURCES := qtkt.c

qtknconv_SOURCES := \
	qt-conv.c qtkn.c  ../lib/path_helper.c ../lib/progress_bar.c

qtknconv_CC65SOURCES := ../lib/extrazp.s qtk_bithuff.s \
	../lib/mult16x16x32.s \
	../lib/approxdiv16x8.s \
	../lib/square_table.s
	
qtknconv_GCCSOURCES := qtk_bithuff.c

jpegconv_SOURCES := \
	qt-conv.c jpeg.c ../lib/path_helper.c ../lib/progress_bar.c

jpegconv_CC65SOURCES := jpeg_platform.s ../lib/extrazp.s
jpegconv_GCCSOURCES := jpeg_platform.c

GCCSOURCES := ../lib/extended_conio.c ../lib/clrzone.c
DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/quicktake.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(USE_LANGCARD) $(debug_CFLAGS) $(iigs_CFLAGS)

slowtake_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-hgr.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0xC00 #One file buffer
qtktconv_CLCFLAGS = -D_CACHE_SIZE=1024 --asm-define CACHE_SIZE=1024 $(CLCFLAGS) -C ../../config/apple2enh-alignbss.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000    #Two file buffers
qtknconv_CLCFLAGS = -D_CACHE_SIZE=4096 --asm-define CACHE_SIZE=4096 $(CLCFLAGS) -C ../../config/apple2enh-aligndata.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000
jpegconv_CLCFLAGS = -D_CACHE_SIZE=4096 --asm-define CACHE_SIZE=4096 $(CLCFLAGS) -C ../../config/apple2enh.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000

GCCCFLAGS = -D_CACHE_SIZE=4096 -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: \
	splash.h \
	slowtake.bin slowtake.x86_64 \
	qtktconv.bin qtktconv.x86_64 \
	qtknconv.bin qtknconv.x86_64 \
	jpegconv.bin jpegconv.x86_64 \
	loader.x86_64

splash.h: splash.hgr
	./make-splash.sh

slowtake.bin: $(slowtake_SOURCES) $(slowtake_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"slowtake.dbg" $(CFLAGS) $(slowtake_CLCFLAGS) -vm -m slowtake.map -o $@ $^

slowtake.x86_64: $(slowtake_SOURCES) $(slowtake_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

qtktconv.bin: $(qtktconv_CC65SOURCES) $(qtktconv_SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qtktconv.dbg" $(CFLAGS) $(qtktconv_CLCFLAGS) -vm -m qtktconv.map -o $@ $^

qtktconv.x86_64: $(qtktconv_SOURCES) $(qtktconv_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

qtknconv.bin: $(qtknconv_SOURCES) $(qtknconv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qtknconv.dbg" $(CFLAGS) $(qtknconv_CLCFLAGS) -vm -m qtknconv.map -o $@ $^

qtknconv.x86_64: $(qtknconv_SOURCES) $(qtknconv_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

jpegconv.bin: $(jpegconv_SOURCES) $(jpegconv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"jpegconv.dbg" $(CFLAGS) $(jpegconv_CLCFLAGS) -vm -m jpegconv.map -o $@ $^

jpegconv.x86_64: $(jpegconv_SOURCES) $(jpegconv_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

loader.x86_64: loader.c
	gcc -o $@ -I/usr/include/SDL loader.c ../lib/extended_conio.c ../lib/clrzone.c -lSDL_image -lSDL

clean:
	rm -f *.bin *.x86_64 *.o *.map splash.h

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ && \
	cp *.bin about /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/
