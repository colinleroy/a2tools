CC65_TARGET = apple2enh
ifdef IIGS
serial_hw := z8530
iigs_suffix := -iigs
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
upload_subdir := /iigs
else
ifdef OLDII
serial_hw := acia
iigs_FLAGS :=
suffix :=
upload_subdir :=
CC65_TARGET := apple2
else
serial_hw := acia
iigs_suffix :=
iigs_CFLAGS :=
CC65_TARGET = apple2enh
trgt = apple2c0
endif
endif

PROGRAM=wozamp
SOURCES := \
	main.c \
	../lib/hgr.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_configure.c \
	../lib/path_helper.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_wait_for_stream.c \
	../lib/surl/surl_receive.c \
	../lib/surl/surl_ping.c \
	../lib/malloc0.c \
	../lib/dgets.c \
	../lib/scrollwindow.c \
	../lib/strsplit.c \
	../stp/stp_list.c \
	../lib/strcasestr.c

CL65SOURCES = \
	../lib/pwm.s \
	../lib/surl/surl_read_with_barrier.s \
	../lib/serial/serial_read_no_irq.s \
	../lib/serial/simple_serial_read.s \
	../lib/serial/simple_serial_io.s \
	../lib/serial/$(serial_hw)/common.s \
	../lib/serial/$(serial_hw)/control.s \
	../lib/serial/$(serial_hw)/no_irq.s \
 ../lib/dputc.s \
 ../lib/dputs.s \
 ../lib/scroll.s \
 ../lib/FVTABZ.s \
 ../lib/fastirq$(suffix).s \
 ../lib/runtime_once_clean.s \
 ../lib/extrazp.s \
 ../lib/clrzone.s

GCC_SOURCES = \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/clrzone.c \
	../lib/serial/simple_serial_io.c

#AVOID_ROM_CALLS Needed for scrolldown in 40cols mode
CLCFLAGS = -t $(CC65_TARGET) -DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-DAVOID_ROM_CALLS --asm-define AVOID_ROM_CALLS \
	-DSTP_DATA_SIZE=8192 \
	-I ../lib -I ../stp \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-pwmstreamer.cfg \
	-vm -m $(PROGRAM).map -DSURL_TO_LANGCARD -O

.PHONY: all clean

all: $(PROGRAM).bin

$(PROGRAM).bin: $(CL65SOURCES) $(SOURCES)
	cl65 -g -Wl --dbgfile,$(PROGRAM).dbg -Ln $(PROGRAM).lbl $(CFLAGS) $(iigs_CFLAGS) $(CLCFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAM).bin *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/wozamp/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/wozamp/ && \
	cp wozamp.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/wozamp/
