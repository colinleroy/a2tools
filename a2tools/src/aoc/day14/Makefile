PROGRAM=day14.bin day14x2.bin
SOURCES := day14.c
DEPS := $(topdir)../../lib/bool_array.o \
	$(topdir)../../lib/extended_string.o \
	$(topdir)../../../drivers/a2.hi.o \

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib
LDFLAGS = -t $(CC65_TARGET) 

.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

day14.bin: day14.o
	$(CC) $(LDFLAGS) -m day14.bin.map -C ../../../drivers/apple2-hgr.cfg -o $@ $^ $(DEPS)

day14x2.bin: day14-part2.o
	$(CC) $(LDFLAGS) -m day14_2.bin.map -o $@ $^ $(DEPS)

clean:
	rm -f $(PROGRAM) *.o *.map
