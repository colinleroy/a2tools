TARGET := apple2

GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	-DDYN_BOX_DRAW -DAPPLE2_INCLUDE_IIE_CHARS \

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -I ../stp -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror -DSTP_DATA_SIZE=8192

FLOP_NAME = WEATHER
CLEANDISK = ../../disks/ProDOS_2_4_3.po

.PHONY: all clean

weather_SOURCES := \
	hgrtext.s \
	ipapi.c \
	main.c \
	openmeteo.c \
	weatherdisp.c \
	weatherui.c \
	../lib/FVTABZ.s \
	../lib/surl/asm/surl_ping.s \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_get_json.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/malloc0.s \
	../lib/citoa.s \
	../lib/cntoabuf.s \
	../lib/a2_features.s \
	../lib/strsplit.c


weather_CLCFLAGS = -t $(TARGET) -I ../lib \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-rtonce.cfg \
	-vm -m weather.map $(GENERAL_CFLAGS) -O --start-addr 0x4000

all: weather.po

add_disk:
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)WEATHER.SYSTEM SYS < ../../bin/loader.system; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $(IMG) $(PREFIX)$$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

weather.po: weather.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ $(FLOP_NAME)
	IMG=$@ PREFIX= make add_disk

hdv: 
	java -jar ../../bin/ac.jar -d $(IMG) $(FLOP_NAME)
	PREFIX=$(FLOP_NAME)/ make add_disk

clean:
	rm -f *.bin *.o *.map *.dbg *.lbl *.po

weather.bin: $(weather_CL65SOURCES) $(weather_SOURCES)
	cl65 -g -Wl --dbgfile,weather.dbg -Ln weather.lbl $(CFLAGS) $(weather_CLCFLAGS) -o $@ $^  $(TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/weather/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/weather/$(suffix)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/weather/$(suffix)/

dist: weather.po
	mkdir -p ../../dist/
	cp $< ../../dist/
