TARGET := apple2

GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	-DAVOID_ROM_CALLS \
	--asm-define AVOID_ROM_CALLS \
	--asm-define SERIAL_ENABLE_IRQ \
	-DDYN_BOX_DRAW -DAPPLE2_INCLUDE_IIE_CHARS \

FLOP_NAME = TELNET
CLEANDISK = ../../disks/ProDOS_2_4_3.po

PROGRAM=telnet
SOURCES := \
	telnet.c \
	../lib/dget_text_single.c

#Fastirq is needed in telnet

CC65SOURCES := \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_getc_immediate.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/c/simple_serial_printf.c \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/dputs.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/extrazp.s \
	../lib/fastirq.s \
	../lib/fastirq-iigs.s \
	../lib/a2_features.s \

GCCSOURCES := \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/serial/c/simple_serial_printf.c \
	../lib/surl/c/surl_core.c \
	../lib/c/scrollwindow.c \
	../lib/c/path_helper.c \
	../lib/c/malloc0.c \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/c/clrzone.c

DEPS := 

CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(TARGET) -Osir -Cl -C ../../config/apple2enh-rtonce.cfg \
	$(GENERAL_CFLAGS)
GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-function -Wno-unknown-pragmas -Werror
GCCLDFLAGS = 

.PHONY: all clean

all: telnet.po

add_disk:
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)TELNET.SYSTEM SYS < ../../bin/loader.system; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $(IMG) $(PREFIX)$$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

telnet.po: $(PROGRAM).bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ $(FLOP_NAME)
	IMG=$@ PREFIX= make add_disk

hdv: 
	java -jar ../../bin/ac.jar -d $(IMG) $(FLOP_NAME)
	PREFIX=$(FLOP_NAME)/ make add_disk

$(PROGRAM).bin: $(SOURCES) $(CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"telnet.dbg" -Ln telnet.lbl $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map *.dbg *.lbl *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/

dist: telnet.po
	mkdir -p ../../dist/
	cp $< ../../dist/
