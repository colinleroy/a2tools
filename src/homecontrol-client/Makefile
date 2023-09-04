PROGRAM=homectrl grphview
homectrl_SOURCES := main.c network.c \
	switches.c sensors.c climate.c \
	server_url.c \
	../lib/simple_serial.c \
	../lib/extended_conio.c \
	../lib/dgets.c \
	../lib/scrollwindow.c \
	../lib/clrzone.c \
	../lib/strsplit.c \
	../lib/extended_string.c \
	../lib/surl_core.c \
	../lib/surl_receive.c

homectrl_CL65SOURCES = ../lib/dputc.s ../lib/scroll.s ../lib/fastirq.s

grphview_SOURCES := graph-viewer.c \
	../lib/surl_core.c \
	../lib/surl_receive_lines.c \
	../lib/simple_serial.c

grphview_GCCSOURCES := ../lib/extended_conio.c ../lib/clrzone.c ../lib/tgi_sdl.c ../lib/extended_string.c ../lib/tgi_fastline.c
grphview_CL65SOURCES = ../lib/fastirq.s

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
homectrl_CLCFLAGS = -t $(CC65_TARGET) -O -Wl -D,__STACKSIZE__=0x0400 -DSURL_TO_LANGCARD
grphview_CLCFLAGS = -t $(CC65_TARGET) -O -Wl -D,__STACKSIZE__=0x0400 -DSURL_TO_LANGCARD --start-addr 0x4000 #-C ../../config/apple2enh-hgr.cfg
GCCCFLAGS = -g -O0 -Wall -Wno-unused-function -Wno-char-subscripts -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)
.PHONY: all clean

all: homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64

homectrl.bin: $(homectrl_SOURCES) $(homectrl_CL65SOURCES)
	cl65 $(CFLAGS) $(homectrl_CLCFLAGS) -m homectrl.map -o $@ $^

homectrl.x86_64: $(homectrl_SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

grphview.bin: $(grphview_SOURCES) $(grphview_CL65SOURCES)
	cl65 $(CFLAGS) $(grphview_CLCFLAGS) -m grphview.map -o $@ $^

grphview.x86_64: $(grphview_SOURCES) $(grphview_GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/homecontrol/ || \
	mkdir /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/homecontrol/ && \
	cp homectrl.bin grphview.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/homecontrol/
