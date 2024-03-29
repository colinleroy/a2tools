ifdef IIGS
serial_hw_SOURCES := ../lib/serial/z8530/common.s ../lib/serial/z8530/no_irq.s
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs
else
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
iigs_FLAGS :=
suffix :=
upload_subdir :=
endif

PROGRAM=telnet
SOURCES := \
	telnet.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_printf.c \
	../lib/surl/surl_core.c \
	../lib/dgets.c \
	../lib/scrollwindow.c \
	../lib/path_helper.c \
	../lib/malloc0.c

CC65SOURCES := \
	../lib/surl/surl_read_with_barrier.s \
	../lib/serial/serial_read_no_irq.s \
	../lib/serial/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	../lib/clrzone.s \
	../lib/dputs.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/fastirq$(suffix).s \
	../lib/extrazp.s

GCCSOURCES := \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/clrzone.c \
	../lib/serial/simple_serial_io.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -Osir -Cl $(iigs_CFLAGS) -C ../../config/apple2enh-rtonce.cfg -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD  -DAVOID_ROM_CALLS --asm-define AVOID_ROM_CALLS
GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-function -Wno-unknown-pragmas -Werror
GCCLDFLAGS = 

.PHONY: all clean

all: $(PROGRAM).bin

$(PROGRAM).bin: $(SOURCES) $(CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"telnet.dbg" -Ln telnet.lbl $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/
