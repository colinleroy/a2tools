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

qtmenu_SOURCES := \
	qt-menu.c qt-serial.c qt1x0-serial.c qt-edit-image.c \
	../lib/dgets.c ../lib/scrollwindow.c ../lib/path_helper.c \
	../lib/simple_serial.c ../lib/simple_serial_ext.c \
	../lib/extended_string.c ../lib/malloc0.c \
	../lib/file_select.c ../lib/progress_bar.c ../lib/hgr.c \
	../lib/scrollwindow.c

qtmenu_CC65SOURCES := \
	../lib/dputc.s ../lib/dputs.s ../lib/scroll.s \
	../lib/clrzone.c ../lib/fastirq$(suffix).s \
	../lib/platform_sleep.c

qt100conv_SOURCES := \
	qt-conv.c qt100.c  ../lib/path_helper.c ../lib/progress_bar.c
qt100conv_CC65SOURCES :=

qt150conv_SOURCES := \
	qt-conv.c qt150.c  ../lib/path_helper.c ../lib/progress_bar.c
qt150conv_CC65SOURCES :=

GCCSOURCES := ../lib/extended_conio.c ../lib/clrzone.c
DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/quicktake.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(USE_LANGCARD) $(debug_CFLAGS) $(iigs_CFLAGS)

qtmenu_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-hgr.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000
qt100conv_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000
qt150conv_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000

GCCCFLAGS = -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: \
	splash.h \
	qtmenu.bin qtmenu.x86_64 \
	qt100conv.bin qt100conv.x86_64 \
	qt150conv.bin qt150conv.x86_64

splash.h: splash.hgr
	./make-splash.sh

qtmenu.bin: $(qtmenu_SOURCES) $(qtmenu_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qtmenu.dbg" $(CFLAGS) $(qtmenu_CLCFLAGS) -vm -m qtmenu.map -o $@ $^

qtmenu.x86_64: $(qtmenu_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

qt100conv.bin: $(qt100conv_SOURCES) $(qt100conv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qt100conv.dbg" $(CFLAGS) $(qt100conv_CLCFLAGS) -vm -m qt100conv.map -o $@ $^

qt100conv.x86_64: $(qt100conv_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

qt150conv.bin: $(qt150conv_SOURCES) $(qt150conv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qt150conv.dbg" $(CFLAGS) $(qt150conv_CLCFLAGS) -vm -m qt150conv.map -o $@ $^

qt150conv.x86_64: $(qt150conv_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

loader.x86_64:
	gcc -o $@ -I/usr/include/SDL loader.c ../lib/extended_conio.c ../lib/clrzone.c -lSDL_image -lSDL

clean:
	rm -f *.bin *.x86_64 *.o *.map splash.h

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/
