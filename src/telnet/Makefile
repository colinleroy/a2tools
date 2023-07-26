PROGRAM=telnet
SOURCES := telnet.c ../lib/simple_serial.c ../lib/surl_core.c ../lib/clrzone.c ../lib/dgets.c ../lib/scrollwindow.c
CC65SOURCES := ../lib/dputs.s ../lib/dputc.s ../lib/scroll.s
GCCSOURCES := ../lib/extended_string.c ../lib/extended_conio.c
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -Osir -Cl
GCCCFLAGS = -g -O0 -Wall 
GCCLDFLAGS = 

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

$(PROGRAM).bin: $(SOURCES) $(CC65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map

upload: all
	scp $(PROGRAM).bin diskstation.lan:/volume1/a2repo/
