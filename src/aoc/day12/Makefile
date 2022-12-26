PROGRAM=day12.bin
SOURCES := day12.c
DEPS := ../../lib/bfs.o

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib
LDFLAGS = -t $(CC65_TARGET) -m $(PROGRAM).map

.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -o $@ $^ $(DEPS)

clean:
	rm -f $(PROGRAM) *.o *.map
