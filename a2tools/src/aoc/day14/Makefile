PROGRAM=day14.bin
SOURCES := day14.c
DEPS := $(topdir)../../lib/slist.o \
	$(topdir)../../lib/bool_array.o \
	$(topdir)../../lib/extended_string.o \
	$(topdir)../../../drivers/a2.hi.o \

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -C ../../../drivers/apple2-hgr.cfg -I ../../lib
LDFLAGS = -t $(CC65_TARGET) -C ../../../drivers/apple2-hgr.cfg -m $(PROGRAM).map

.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -o $@ $^ $(DEPS)

clean:
	rm -f $(PROGRAM) *.o *.map
