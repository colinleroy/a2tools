ifdef IIGS
serial_hw_SOURCES := ../lib/serial/z8530/common.s ../lib/serial/z8530/no_irq.s
iigs_suffix := -iigs
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
CC65_TARGET = apple2enh
upload_subdir := /iigs
else
ifdef OLDII
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
iigs_FLAGS :=
suffix :=
upload_subdir :=
CC65_TARGET := apple2
STREAMER_VER := 6502
else
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
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

test_SOURCES := \
	main.s \
	mouse.s \
	palette.s \
	draw.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s \
	../lib/platform_sleep.s \
	../lib/asm/clrzone.s \
	../lib/FVTABZ.s \
	../lib/fastirq.s \
	../lib/extrazp.s \
	../lib/asm/hgr.s \
	../lib/hgr_addrs.c

test_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl -C ../../config/apple2enh-rtonce.cfg --start-addr 0x6000

all: \
	test.bin

clean:
	rm -f *.bin *.o *.map splash.h splash.hgr

test.bin: $(test_CL65SOURCES) $(test_SOURCES)
	cl65 -g -Wl --dbgfile,test.dbg -Ln test.lbl $(CFLAGS) $(iigs_CFLAGS) $(test_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/
