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

imageview_SOURCES := \
	imageview.c \
	../lib/simple_serial.c ../lib/path_helper.c ../lib/extended_string.c \
	../lib/file_select.c ../lib/progress_bar.c ../lib/hgr.c \
	../lib/scrollwindow.c \
	../lib/malloc0.c

imageview_CC65SOURCES := \
	../lib/dputc.s ../lib/scroll.s \
	../lib/clrzone.c ../lib/fastirq$(suffix).s \
	../lib/platform_sleep.c ../lib/extrazp.s

imageview_GCCSOURCES := ../lib/tgi_sdl.c ../lib/tgi_fastline.c

GCCSOURCES := ../lib/extended_conio.c ../lib/clrzone.c
DEPS :=

CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(USE_LANGCARD) $(debug_CFLAGS) $(iigs_CFLAGS)

imageview_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh-hgr.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0xC00 #One file buffer

GCCCFLAGS = -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: \
	imageview.bin imageview.x86_64

imageview.bin: $(imageview_SOURCES) $(imageview_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"imageview.dbg" $(CFLAGS) $(imageview_CLCFLAGS) -vm -m imageview.map -o $@ $^

imageview.x86_64: $(imageview_SOURCES) $(imageview_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f *.bin *.x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/imageview/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/imageview/ && \
	cp *.bin about.hgr /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/imageview/
