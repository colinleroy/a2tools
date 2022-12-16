PROGRAM=day16.bin
SOURCES := day16.c
DEPS := ../../lib/slist.o

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib
LDFLAGS = -t $(CC65_TARGET) 

.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -m $(PROGRAM).map -o $@ $^ $(DEPS)

clean:
	rm -f $(PROGRAM) *.o *.map
