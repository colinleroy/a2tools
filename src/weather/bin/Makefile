GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I $(SRC_DIR)/../lib -I $(SRC_DIR)/../stp -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror -DSTP_DATA_SIZE=8192

.PHONY: all clean

weather_SOURCES := \
	$(SRC_DIR)/hgrtext.s \
	$(SRC_DIR)/ipapi.c \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/openmeteo.c \
	$(SRC_DIR)/weatherdisp.c \
	$(SRC_DIR)/weatherui.c \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/surl/asm/surl_ping.s \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_get_json.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(SRC_DIR)/../lib/serial/asm/driver.s \
	$(SRC_DIR)/../lib/serial/asm/acia.s \
	$(SRC_DIR)/../lib/serial/asm/z8530.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_configure.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/citoa.s \
	$(SRC_DIR)/../lib/cntoabuf.s \
	$(SRC_DIR)/../lib/a2_features.s \
	$(SRC_DIR)/../lib/strsplit.c


weather_CLCFLAGS = -t $(CC65_TARGET) -I $(SRC_DIR)/../lib \
	-Wl -D,__STACKSIZE__=0x0400 -C $(SRC_DIR)/../../config/apple2enh-rtonce.cfg \
	-vm -m weather.map $(GENERAL_CFLAGS) -O --start-addr 0x4000

all: weather$(iigs_suffix).po

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
weather$(iigs_suffix).po: weather.bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ WEATHER
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ WEATHER.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	for r in *.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

clean:
	rm -f *.bin *.o *.map *.dbg *.lbl *.po

weather.bin: $(weather_CL65SOURCES) $(weather_SOURCES)
	cl65 -g -Wl --dbgfile,weather.dbg -Ln weather.lbl $(CFLAGS) $(weather_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/weather/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/weather/$(suffix)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/weather/$(suffix)/

dist: weather$(iigs_suffix).po
	mkdir -p $(SRC_DIR)../../dist/
	cp $< $(SRC_DIR)../../dist/
