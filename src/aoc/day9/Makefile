PROGRAM=day9
SOURCES := day9.c ../../lib/bool_array.c
GCCSOURCES := ../../lib/extended_conio.c ../../lib/clrzone.c
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../../lib
CLCFLAGS = -t $(CC65_TARGET)
GCCCFLAGS = -g -O0 -Werror

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

$(PROGRAM).bin: $(SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map
