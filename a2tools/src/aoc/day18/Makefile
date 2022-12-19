PROGRAM=day18.bin
SOURCES := day18.c
DEPS := 

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib #-C ../../../drivers/apple2-system.cfg
LDFLAGS = -t $(CC65_TARGET) 

.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -m $(PROGRAM).map -o $@ $^ $(DEPS)

stacktest.o: stacktest.c
	$(CC) -c $(CFLAGS) -o $@ $<

stacktest.bin: stacktest.o
	$(CC) $(LDFLAGS) -m stacktest.map -o $@ $^ $(DEPS)

clean:
	rm -f $(PROGRAM) *.o *.map
