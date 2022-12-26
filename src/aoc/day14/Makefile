PROGRAM=day14.bin
SOURCES := day14.c ../../lib/bool_array.c ../../lib/extended_string.c
DEPS := 

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib -C ../../../config/apple2-hgr.cfg 

.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	$(CC) $(CFLAGS) -m $(PROGRAM).map -o $@ $^

clean:
	rm -f $(PROGRAM) *.o *.map
