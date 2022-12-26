READER=day21rd.bin
READER_SOURCES := day21rd.c ../../lib/bigint.o

WRITER=day21wr.bin
WRITER_SOURCES := day21wr.c ../../lib/bigint.o

DEPS := 

CC65_TARGET = apple2
CC      = cl65
READER_CFLAGS  = -t $(CC65_TARGET) -I ../../lib  -Wl -D,__STACKSIZE__=0x6100 --check-stack
WRITER_CFLAGS  = -t $(CC65_TARGET) -I ../../lib

.PHONY: all clean
all: $(READER) $(WRITER)

$(READER): $(READER_SOURCES)
	$(CC) $(READER_CFLAGS) -m $(PROGRAM).map -o $@ $^

$(WRITER): $(WRITER_SOURCES)
	$(CC) $(WRITER_CFLAGS) -m $(PROGRAM).map -o $@ $^

clean:
	rm -f $(READER) $(WRITER) *.o *.map
