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

PROGRAM=homectrl grphview
homectrl_SOURCES := main.c network.c \
	switches.c sensors.c climate.c \
	server_url.c \
	../lib/malloc0.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_configure.c \
	../lib/extended_conio.c \
	../lib/dgets.c \
	../lib/scrollwindow.c \
	../lib/strsplit.c \
	../lib/strellipsis.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_receive.c \
	../lib/path_helper.c \
	../lib/surl/surl_ping.c
	
	homectrl_CL65SOURCES = \
	../lib/surl/surl_read_with_barrier.s \
	../lib/serial/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	../lib/dputc.s\
	../lib/dputs.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/fastirq$(suffix).s \
	../lib/extrazp.s \
	../lib/clrzone.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s


homectrl_GCCSOURCES = \
	../lib/strtrim.c \
	../lib/clrzone.c \
	../lib/serial/simple_serial_io.c

grphview_SOURCES := graph-viewer.c \
	../lib/malloc0.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_receive_lines.c \
	../lib/serial/simple_serial.c \
	../lib/path_helper.c
	
	grphview_CL65SOURCES = \
	../lib/surl/surl_read_with_barrier.s \
	../lib/serial/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	../lib/clrzone.s \
	../lib/fastirq$(suffix).s \
	../lib/extrazp.s \
	../lib/FVTABZ.s

grphview_GCCSOURCES := \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/clrzone.c \
	../lib/tgi_sdl.c \
	../lib/tgi_fastline.c \
	../lib/serial/simple_serial_io.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
homectrl_CLCFLAGS = -t $(CC65_TARGET) -O -Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-rtonce.cfg -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD $(iigs_CFLAGS)
grphview_CLCFLAGS = $(homectrl_CLCFLAGS) --start-addr 0x4000 #-C ../../config/apple2enh-hgr.cfg
GCCCFLAGS = -g -O0 -Wall  -Wno-unknown-pragmas -Wno-unused-function -Wno-char-subscripts -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)
.PHONY: all clean

all: homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64

homectrl.bin: $(homectrl_SOURCES) $(homectrl_CL65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"homectrl.dbg" $(CFLAGS) $(homectrl_CLCFLAGS) -m homectrl.map -o $@ $^

homectrl.x86_64: $(homectrl_SOURCES) $(homectrl_GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

grphview.bin: $(grphview_SOURCES) $(grphview_CL65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"grphview.dbg" $(CFLAGS) $(grphview_CLCFLAGS) -m grphview.map -o $@ $^

grphview.x86_64: $(grphview_SOURCES) $(grphview_GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/homecontrol/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/homecontrol/ && \
	cp homectrl.bin grphview.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/homecontrol/
