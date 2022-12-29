PROGRAM=day15
SOURCES := day15.c ../../lib/slist.c ../../lib/array_sort.c
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../../lib
CLCFLAGS = -t $(CC65_TARGET)
GCCCFLAGS = -g -O0 -Werror

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

$(PROGRAM).bin: $(SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map
