fCC65_TARGET = apple2enh
ifdef IIGS
serial_hw := z8530
iigs_suffix := -iigs
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
upload_subdir := /iigs
CC65_TARGET = apple2enh
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

USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -I ../stp -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror -DSTP_DATA_SIZE=8192

.PHONY: all clean

weather_SOURCES := \
	hgrtext.s \
	ipapi.c \
	main.c \
	openmeteo.c \
	weatherdisp.c \
	weatherui.c \
	../lib/FVTABZ.s \
	../lib/surl/surl_ping.c \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_get_json.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/serial/$(serial_hw)/common.s \
	../lib/serial/$(serial_hw)/no_irq.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/malloc0.s \
	../lib/citoa.s \
	../lib/cntoabuf.s \
	../lib/strsplit.c


weather_CLCFLAGS = -t $(CC65_TARGET) -I ../lib \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-rtonce.cfg \
	-vm -m weather.map $(USE_LANGCARD) -O --start-addr 0x4000

all: \
	weather.bin

clean:
	rm -f *.bin *.o *.map

weather.bin: $(weather_CL65SOURCES) $(weather_SOURCES)
	cl65 -g -Wl --dbgfile,weather.dbg -Ln weather.lbl $(CFLAGS) $(iigs_CFLAGS) $(weather_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/weather/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/weather/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/weather/
