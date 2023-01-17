PROGRAM=mirror.bin
SOURCES := mirror.c ../lib/simple_serial.c
DEPS := 

CC65_TARGET = apple2enh
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../lib
LDFLAGS = -t $(CC65_TARGET) -m $(PROGRAM).map

.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -o $@ $^ $(DEPS)

clean:
	rm -f $(PROGRAM) *.o *.map
