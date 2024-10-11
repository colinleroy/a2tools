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

USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

PROGRAM=homectrl grphview
homectrl_SOURCES := main.c network.c \
	switches.c sensors.c climate.c \
	server_url.c \
	../lib/extended_conio.c \
	../lib/dgets.c \
	../lib/strsplit.c \
	../lib/strellipsis.c \
	../lib/surl/surl_ping.c
	
homectrl_CL65SOURCES = \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/serial/asm/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	../lib/surl/asm/surl_core.s \
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
	#../lib/fastirq$(suffix).s \

homectrl_GCCSOURCES = \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/surl/c/surl_core.c \
	../lib/surl/c/surl_receive.c \
	../lib/c/scrollwindow.c \
	../lib/c/malloc0.c \
	../lib/c/path_helper.c \
	../lib/c/clrzone.c \
	../lib/strtrim.c

grphview_SOURCES := graph-viewer.c \
	../lib/surl/surl_receive_lines.c
	
grphview_CL65SOURCES = \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/fastirq$(suffix).s \
	../lib/extrazp.s \
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

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
homectrl_CLCFLAGS = -t $(CC65_TARGET) -O -Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-rtonce.cfg $(USE_LANGCARD) $(iigs_CFLAGS)
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
