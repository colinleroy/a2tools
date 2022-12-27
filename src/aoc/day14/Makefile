PROGRAM=day14
SOURCES := day14.c ../../lib/bool_array.c ../../lib/extended_string.c ../../lib/tgi_sdl.c
DEPS := 

CC65_TARGET = apple2
CFLAGS  = -I ../../lib
CLCFLAGS = -t $(CC65_TARGET) -C ../../../config/apple2-hgr.cfg 
GCCCFLAGS = -g -O0 -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

$(PROGRAM).bin: $(SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@ 

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map
