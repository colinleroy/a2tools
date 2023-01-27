READER=day21rd
READER_SOURCES := day21rd.c ../../lib/bigint.c
READER_GCCSOURCES := ../../lib/extended_conio.c ../../lib/clrzone.c

WRITER=day21wr
WRITER_SOURCES := day21wr.c ../../lib/bigint.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS = -I ../../lib
GCCCFLAGS = -g -O0 -Werror
READER_CLCFLAGS  = -t $(CC65_TARGET)  -Wl -D,__STACKSIZE__=0x6100 --check-stack
WRITER_CLCFLAGS  = -t $(CC65_TARGET)

.PHONY: all clean
all: $(READER).bin $(READER).x86_64 $(WRITER).bin $(WRITER).x86_64

$(READER).bin: $(READER_SOURCES)
	cl65 $(READER_CLCFLAGS) $(CFLAGS) -m $(READER).map -o $@ $^

$(WRITER).bin: $(WRITER_SOURCES)
	cl65 $(WRITER_CLCFLAGS) $(CFLAGS) -m $(WRITER).map -o $@ $^

$(READER).x86_64: $(READER_SOURCES) $(READER_GCCSOURCES)
	gcc $(GCCCFLAGS) $(CFLAGS) -o $@ $^

$(WRITER).x86_64: $(WRITER_SOURCES)
	gcc $(GCCCFLAGS) $(CFLAGS) -o $@ $^

clean:
	rm -f $(READER).bin $(WRITER).bin $(READER).x86_64 $(WRITER).x86_64 *.o *.map
