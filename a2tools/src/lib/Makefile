SOURCES := simple_serial.c \
	extended_conio.c
DEPS := $(topdir)../../drivers/a2.ssc.o

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) 
LDFLAGS = -t $(CC65_TARGET) -m $(PROGRAM).map

.PHONY: all clean
all: $(SOURCES:.c=.o)
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGRAM) *.o *.map
