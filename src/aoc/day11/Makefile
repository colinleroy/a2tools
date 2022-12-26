PROGRAM=day11.bin
SOURCES := day11.c ../../lib/array_sort.c ../../lib/extended_string.c ../../lib/bigint.o
DEPS := 

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib

.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	$(CC) $(CFLAGS) -m $(PROGRAM).map -o $@ $^

clean:
	rm -f $(PROGRAM) *.o *.map
