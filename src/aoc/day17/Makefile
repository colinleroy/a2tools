PROGRAM=day17.bin
SOURCES := day17.c
DEPS := ../../lib/bool_array.o ../../lib/extended_conio.o

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib -Oi -Os
LDFLAGS = -t $(CC65_TARGET) 

.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -m $(PROGRAM).map -o $@ $^ $(DEPS)

clean:
	rm -f $(PROGRAM) *.o *.map
