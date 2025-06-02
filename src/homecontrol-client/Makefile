TARGET := apple2
GENERAL_CFLAGS := -DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	-DDYN_BOX_DRAW -DAPPLE2_INCLUDE_IIE_CHARS \

PROGRAM=homectrl grphview
homectrl_SOURCES := \
	main.c \
	network.c \
	switches.c \
	sensors.c \
	climate.c \
	server_url.c \
	../lib/extended_conio.c \
	../lib/dget_text_single.c \
	../lib/strsplit.c \
	../lib/strellipsis.c \
	
homectrl_CL65SOURCES = \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_ping.s \
	../lib/surl/asm/surl_receive.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/dputc.s\
	../lib/dputs.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/extrazp.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s \
	../lib/a2_features.s \

homectrl_GCCSOURCES = \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/surl/c/surl_core.c \
	../lib/surl/c/surl_ping.c \
	../lib/surl/c/surl_receive.c \
	../lib/c/scrollwindow.c \
	../lib/c/malloc0.c \
	../lib/c/path_helper.c \
	../lib/c/clrzone.c \
	../lib/strtrim.c

grphview_SOURCES := \
	graph-viewer.c \
	../lib/surl/surl_receive_lines.c
	
grphview_CL65SOURCES = \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/extrazp.s \
	../lib/a2_features.s \
	../lib/FVTABZ.s

grphview_GCCSOURCES := \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/surl/c/surl_core.c \
	../lib/c/path_helper.c \
	../lib/c/clrzone.c \
	../lib/c/malloc0.c \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/tgi_sdl.c \
	../lib/tgi_fastline.c

DEPS := 

CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
homectrl_CLCFLAGS = -t $(TARGET) -O -Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-rtonce.cfg $(GENERAL_CFLAGS)
grphview_CLCFLAGS = $(homectrl_CLCFLAGS) --start-addr 0x4000
GCCCFLAGS = -g -O0 -Wall  -Wno-unknown-pragmas -Wno-unused-function -Wno-char-subscripts -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)
.PHONY: all clean

all: homecontrol.po
	
CLEANDISK = ../../disks/ProDOS_2_4_3.po
homecontrol.po: homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ HOMECONTROL
	java -jar ../../bin/ac.jar -p $@ HOMECTRL.SYSTEM SYS < ../../bin/loader.system; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

homecontrolperso.po: homecontrol.po
	cp $< $@; \
	java -jar ../../bin/ac.jar -p $@ SRVURL TXT < SRVURL; \

homectrl.bin: $(homectrl_SOURCES) $(homectrl_CL65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"homectrl.dbg" $(CFLAGS) $(homectrl_CLCFLAGS) -m homectrl.map -o $@ $^

homectrl.x86_64: $(homectrl_SOURCES) $(homectrl_GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

grphview.bin: $(grphview_SOURCES) $(grphview_CL65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"grphview.dbg" $(CFLAGS) $(grphview_CLCFLAGS) -m grphview.map -o $@ $^

grphview.x86_64: $(grphview_SOURCES) $(grphview_GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64 *.o *.map *.dbg *.lbl *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/homecontrol/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/homecontrol/ && \
	cp homectrl.bin grphview.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/homecontrol/

dist:
	true
