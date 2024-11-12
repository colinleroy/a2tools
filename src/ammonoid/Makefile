fCC65_TARGET = apple2enh
ifdef IIGS
serial_hw := z8530
iigs_suffix := -iigs
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
CC65_TARGET = apple2enh
upload_subdir := /iigs
else
ifdef OLDII
serial_hw := acia
iigs_FLAGS :=
suffix :=
upload_subdir :=
CC65_TARGET := apple2
STREAMER_VER := 6502
else
serial_hw := acia
iigs_suffix :=
iigs_CFLAGS :=
CC65_TARGET = apple2enh
trgt = apple2c0
STREAMER_VER := 65c02
endif
endif

USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -I ../stp -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror

.PHONY: all clean

ammonoid_SOURCES := \
	main.c \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/malloc0.s \
	../lib/asm/clrzone.s \
	../lib/dgets.c \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/prodos_dir_file_count.s \
	../lib/FVTABZ.s \

ammonoid_CLCFLAGS = -t $(CC65_TARGET) -I ../lib \
	-DFILESEL_ALLOW_NONPRODOS_VOLUMES \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-rtonce.cfg \
	-vm -m ammonoid.map $(USE_LANGCARD) -O -Cl --start-addr 0x1003

all: \
	ammonoid.bin

clean:
	rm -f *.bin *.o *.map splash.h splash.hgr

ammonoid.bin: $(ammonoid_CL65SOURCES) $(ammonoid_SOURCES)
	cl65 -g -Wl --dbgfile,ammonoid.dbg -Ln ammonoid.lbl $(CFLAGS) $(iigs_CFLAGS) $(ammonoid_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/ammonoid/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/ammonoid/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/ammonoid/
