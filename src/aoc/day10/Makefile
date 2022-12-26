PROGRAM=day10.bin
SOURCES := day10.c ../../lib/bool_array.c
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
