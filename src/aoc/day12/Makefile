PROGRAM=day12 day12viz

RD_SOURCES := day12.c ../../lib/bfs.c ../../lib/bool_array.c
VIZ_SOURCES := day12viz.c ../../lib/tgi_sdl.c
VIZ_GCCSOURCES := ../../lib/extended_conio.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../../lib
RD_CLCFLAGS = -t $(CC65_TARGET)
VIZ_CLCFLAGS = -t $(CC65_TARGET) -C ../../../config/apple2enh-hgr.cfg
RD_GCCCFLAGS = -g -O0 -Werror
VIZ_GCCCFLAGS = -g -O0 -Werror $(shell sdl-config --cflags)
VIZ_GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean
all: day12.bin day12.x86_64 day12viz.bin day12viz.x86_64

day12.bin: $(RD_SOURCES)
	cl65 $(CFLAGS) $(RD_CLCFLAGS) -m day12.map -o $@ $^

day12.x86_64: $(RD_SOURCES)
	gcc $(CFLAGS) $(RD_GCCCFLAGS) $^ $(RD_GCCLDFLAGS) -o $@ 

day12viz.bin: $(VIZ_SOURCES)
	cl65 $(CFLAGS) $(VIZ_CLCFLAGS) -m day12.map -o $@ $^

day12viz.x86_64: $(VIZ_SOURCES) $(VIZ_GCCSOURCES)
	gcc $(CFLAGS) $(VIZ_GCCCFLAGS) $^ $(VIZ_GCCLDFLAGS) -o $@ 

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map
