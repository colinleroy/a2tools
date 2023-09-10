PROGRAM=telnet
SOURCES := telnet.c ../lib/simple_serial.c ../lib/simple_serial_ext.c \
	../lib/surl_core.c ../lib/clrzone.c ../lib/dgets.c ../lib/scrollwindow.c
CC65SOURCES := ../lib/dputs.s ../lib/dputc.s ../lib/scroll.s ../lib/fastirq.s
GCCSOURCES := ../lib/extended_string.c ../lib/extended_conio.c
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -Osir -Cl
GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-function -Werror
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
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ || \
	mkdir /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/
