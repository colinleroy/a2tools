PROGRAM=test
SOURCES := main.c
CLCFLAGS = -t $(CC65_TARGET) -Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-noexit.cfg -vm -m $(PROGRAM).map -DSERIAL_TO_LANGCARD
CC65_TARGET = apple2enh

.PHONY: all clean

all: $(PROGRAM).bin

$(PROGRAM).bin: $(SOURCES) $(CL65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -o $@ $^ 

clean:
	rm -f $(PROGRAM).bin *.o *.map
