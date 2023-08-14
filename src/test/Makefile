PROGRAM=test
SOURCES := main.c ../lib/clrzone.c
CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-noexit.cfg -vm -m $(PROGRAM).map -DSURL_TO_LANGCARD
CC65_TARGET = apple2enh

.PHONY: all clean

all: $(PROGRAM).bin

$(PROGRAM).bin: $(SOURCES) $(CL65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -o $@ $^ 

clean:
	rm -f $(PROGRAM).bin *.o *.map

test: clean all
	cp ../../disks/basic-empty.dsk ./test.dsk && \
	java -jar ../../bin/ac.jar -p test.dsk TEST.SYSTEM SYS < ../../bin/loader.system && \
	java -jar ../../bin/ac.jar -as test.dsk TEST BIN < test.bin && \
	mame apple2c -window -flop1 test.dsk -resolution 560x384 -modem null_modem -bitb socket.localhost:2000 -nomouse -nodebug
