PROGRAM=day21.bin
SOURCES := day21.c
DEPS := ../../lib/bigint.o 

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib #-C ../../../drivers/apple2-system.cfg
LDFLAGS = -t $(CC65_TARGET) 

.PHONY: all clean
all: $(PROGRAM) day21rdr.bin day21wrt.bin

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -m $(PROGRAM).map -o $@ $^ $(DEPS)

day21rdr.bin: day21rdr.o
	$(CC) $(LDFLAGS) -m day21rdr.bin.map -o $@ $^ $(DEPS)

day21wrt.bin: day21wrt.o
	$(CC) $(LDFLAGS) -m day21wrt.bin.map -o $@ $^

clean:
	rm -f $(PROGRAM) *.o *.map
