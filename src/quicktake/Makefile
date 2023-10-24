ifdef SER_DEBUG
ser_debug_SOURCES := ../lib/simple_serial_ext.c
debug_CFLAGS := -DSER_DEBUG=1
else
ser_debug_SOURCES :=
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
	qt-menu.c ../lib/dgets.c ../lib/scrollwindow.c
qtmenu_CC65SOURCES := \
	../lib/dputc.s ../lib/dputs.s ../lib/scroll.s \
	../lib/clrzone.c

qt100conv_SOURCES := \
	qt-conv.c qt100.c
qt100conv_CC65SOURCES :=

qt150conv_SOURCES := \
	qt-conv.c qt150.c
qt150conv_CC65SOURCES :=

GCCSOURCES := ../lib/extended_conio.c ../lib/clrzone.c
DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -I api -I cli -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(USE_LANGCARD) \
	-C ../../config/apple2enh-rtonce.cfg $(debug_CFLAGS) $(iigs_CFLAGS) \
	$(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000

GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: \
	qtmenu.bin qtmenu.x86_64 \
	qt100conv.bin qt100conv.x86_64 \
	qt150conv.bin qt150conv.x86_64

qtmenu.bin: $(qtmenu_SOURCES) $(qtmenu_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qtmenu.dbg" $(CFLAGS) $(CLCFLAGS) -vm -m qtmenu.map -o $@ $^

qtmenu.x86_64: $(qtmenu_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

qt100conv.bin: $(qt100conv_SOURCES) $(qt100conv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qt100conv.dbg" $(CFLAGS) $(CLCFLAGS) -vm -m qt100conv.map -o $@ $^

qt100conv.x86_64: $(qt100conv_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

qt150conv.bin: $(qt150conv_SOURCES) $(qt150conv_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qt150conv.dbg" $(CFLAGS) $(CLCFLAGS) -vm -m qt150conv.map -o $@ $^

qt150conv.x86_64: $(qt150conv_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f *.bin *.x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/quicktake/