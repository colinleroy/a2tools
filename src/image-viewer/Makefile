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

imgview_SOURCES := \
	imageview.c \
	../lib/simple_serial.c ../lib/simple_serial_ext.c \
	../lib/path_helper.c ../lib/extended_string.c \
	../lib/file_select.c ../lib/progress_bar.c ../lib/hgr.c \
	../lib/hgr_addrs.c ../lib/scrollwindow.c \
	../lib/malloc0.c

imgview_CC65SOURCES := \
	../lib/dputc.s ../lib/scroll.s \
	../lib/clrzone.c ../lib/fastirq$(suffix).s \
	../lib/platform_sleep.c ../lib/extrazp.s

imgview_GCCSOURCES := ../lib/tgi_sdl.c ../lib/tgi_fastline.c

GCCSOURCES := ../lib/extended_conio.c ../lib/clrzone.c
DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(USE_LANGCARD) $(debug_CFLAGS) $(iigs_CFLAGS)

imgview_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-rtonce.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x4000

GCCCFLAGS = -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: \
	imgview.bin imgview.x86_64

imgview.bin: $(imgview_SOURCES) $(imgview_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"imgview.dbg" $(CFLAGS) $(imgview_CLCFLAGS) -vm -m imgview.map -o $@ $^

imgview.x86_64: $(imgview_SOURCES) $(imgview_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f *.bin *.x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/imageviewer/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/imageviewer/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/imageviewer/
