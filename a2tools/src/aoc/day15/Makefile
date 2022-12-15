PROGRAM=day15.bin
SOURCES := day15.c
DEPS := ../../lib/slist.o ../../array_sort.o

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib
LDFLAGS = -t $(CC65_TARGET) 

.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

day15.bin: day15.o
	$(CC) $(LDFLAGS) -m day14_2.bin.map -o $@ $^ $(DEPS)

clean:
	rm -f $(PROGRAM) *.o *.map
