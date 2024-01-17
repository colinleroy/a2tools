ifdef IIGS
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
else
iigs_CFLAGS :=
trgt = apple2c0
endif

PROGRAM=test
SOURCES := main.c ../lib/clrzone.s ../lib/scroll.s ../lib/FVTABZ.s ../lib/platform_sleep.s ../lib/speedctrl.s ../lib/dputc.s ../lib/scrollwindow.c
CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh.cfg -vm -m $(PROGRAM).map -DSURL_TO_LANGCARD -O
CC65_TARGET = apple2enh

.PHONY: all clean

all: $(PROGRAM).bin

$(PROGRAM).bin: $(SOURCES) $(CL65SOURCES)
	cl65 -g -Wl --dbgfile,test.dbg -Ln test.lbl $(CFLAGS) $(iigs_CFLAGS) $(CLCFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAM).bin *.o *.map

test: clean all
	cp ../../disks/basic-empty.dsk ./test.dsk && \
	java -jar ../../bin/ac.jar -p test.dsk TEST.SYSTEM SYS < ../../bin/loader.system && \
	java -jar ../../bin/ac.jar -as test.dsk TEST BIN < test.bin && \
	java -jar ../../bin/ac.jar -d test.dsk BASIC.SYSTEM && \
	mame $(trgt) -window -flop1 test.dsk -resolution 560x384 -modem null_modem -bitb socket.localhost:2000 -mouse -debug
