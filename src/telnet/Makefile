ifdef IIGS
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs
else
iigs_FLAGS :=
suffix :=
upload_subdir :=
endif

PROGRAM=telnet
SOURCES := telnet.c ../lib/simple_serial.c ../lib/simple_serial_printf.c \
	../lib/surl_core.c ../lib/clrzone.c ../lib/dgets.c ../lib/scrollwindow.c \
	../lib/path_helper.c ../lib/malloc0.c
CC65SOURCES := ../lib/dputs.s ../lib/dputc.s ../lib/scroll.s ../lib/FVTABZ.s ../lib/fastirq$(suffix).s ../lib/extrazp.s
GCCSOURCES := ../lib/extended_string.c ../lib/extended_conio.c
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -Osir -Cl $(iigs_CFLAGS) -C ../../config/apple2enh-rtonce.cfg -DSURL_TO_LANGCARD
GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-function -Wno-unknown-pragmas -Werror
GCCLDFLAGS = 

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

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
