PROGRAM=day21.bin
SOURCES := day21.c
DEPS := ../../lib/bigint.o 

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../../lib #--check-stack
LDFLAGS = -t $(CC65_TARGET) 

.PHONY: all clean
all: $(PROGRAM) day21rd.bin day21wr.bin

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -m $(PROGRAM).map -o $@ $^ $(DEPS)

day21rd.bin: day21rd.o
	cl65 -t apple2 -m day21rd.bin.map -Wl -D,__STACKSIZE__=0x6100 -Wl -D,__HIMEM__=0xBF00 -o day21rd.bin day21rd.o ../../lib/bigint.o

day21wr.bin: day21wr.o
	$(CC) $(LDFLAGS) -m day21wr.bin.map -o $@ $^

clean:
	rm -f $(PROGRAM) day21rd.bin day21wr.bin *.o *.map
