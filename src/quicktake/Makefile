ifdef SER_DEBUG
debug_CFLAGS := -DSER_DEBUG=1
else
debug_CFLAGS :=
endif

TARGET := apple2enh
USE_LANGCARD := -DSURL_TO_LANGCARD

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
	../lib/dgets.c ../lib/scrollwindow.c ../lib/path_helper.c \
	../lib/simple_serial.c ../lib/simple_serial_ext.c \
	../lib/extended_string.c ../lib/malloc0.c \
	../lib/file_select.c ../lib/progress_bar.c ../lib/hgr.c \
	../lib/scrollwindow.c

slowtake_CC65SOURCES := \
	../lib/dputc.s ../lib/dputs.s ../lib/scroll.s \
	../lib/clrzone.c ../lib/fastirq$(suffix).s \
	../lib/platform_sleep.c ../lib/extrazp.s

qtktconv_SOURCES := \
	qt-conv.c qtkt.c  ../lib/path_helper.c ../lib/progress_bar.c
qtktconv_CC65SOURCES := ../lib/extrazp.s

qtknconv_SOURCES := \
	qt-conv.c qtkn.c  ../lib/path_helper.c ../lib/progress_bar.c
qtknconv_CC65SOURCES := ../lib/extrazp.s

jpegconv_SOURCES := \
	qt-conv.c jpeg.c  ../lib/path_helper.c ../lib/progress_bar.c
jpegconv_CC65SOURCES := ../lib/extrazp.s

GCCSOURCES := ../lib/extended_conio.c ../lib/clrzone.c
DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/quicktake.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(USE_LANGCARD) $(debug_CFLAGS) $(iigs_CFLAGS)

slowtake_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-hgr.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000
qtktconv_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000
qtknconv_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000
jpegconv_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000

GCCCFLAGS = -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: \
	splash.h \
	slowtake.bin slowtake.x86_64 \
	qtktconv.bin qtktconv.x86_64 \
	qtknconv.bin qtknconv.x86_64 \
	jpegconv.bin jpegconv.x86_64

splash.h: splash.hgr
	./make-splash.sh

slowtake.bin: $(slowtake_SOURCES) $(slowtake_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"slowtake.dbg" $(CFLAGS) $(slowtake_CLCFLAGS) -vm -m slowtake.map -o $@ $^

slowtake.x86_64: $(slowtake_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

qtktconv.bin: $(qtktconv_SOURCES) $(qtktconv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qtktconv.dbg" $(CFLAGS) $(qtktconv_CLCFLAGS) -vm -m qtktconv.map -o $@ $^

qtktconv.x86_64: $(qtktconv_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

qtknconv.bin: $(qtknconv_SOURCES) $(qtknconv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qtknconv.dbg" $(CFLAGS) $(qtknconv_CLCFLAGS) -vm -m qtknconv.map -o $@ $^

qtknconv.x86_64: $(qtknconv_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

jpegconv.bin: $(jpegconv_SOURCES) $(jpegconv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"jpegconv.dbg" $(CFLAGS) $(jpegconv_CLCFLAGS) -vm -m jpegconv.map -o $@ $^

jpegconv.x86_64: $(jpegconv_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

loader.x86_64:
	gcc -o $@ -I/usr/include/SDL loader.c ../lib/extended_conio.c ../lib/clrzone.c -lSDL_image -lSDL

clean:
	rm -f *.bin *.x86_64 *.o *.map splash.h

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/
