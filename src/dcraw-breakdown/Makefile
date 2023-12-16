TARGET := apple2enh

dcraw_SOURCES := \
	dcraw.c
dcraw_CC65SOURCES := ../lib/extrazp.s


CC65_TARGET = $(TARGET)
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/quicktake.conf\"

CLCFLAGS = -t $(CC65_TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(USE_LANGCARD)

dcraw_CLCFLAGS = $(CLCFLAGS) -C ../../config/apple2enh.cfg $(CC65_TARGET)-iobuf-0800.o --start-addr 0x1000    #Two file buffers

GCCCFLAGS = -g -O0 -Wall -Wno-pointer-sign -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas $(shell sdl-config --cflags)

.PHONY: all clean

all: \
	dcraw.bin dcraw.x86_64 \
	loader.x86_64

dcraw.bin: $(dcraw_SOURCES) $(dcraw_CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"dcraw.dbg" $(CFLAGS) $(dcraw_CLCFLAGS) -vm -m dcraw.map -o $@ $^

dcraw.x86_64: $(dcraw_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

loader.x86_64:
	gcc -o $@ -I/usr/include/SDL loader.c ../lib/extended_conio.c ../lib/clrzone.c -lSDL_image -lSDL

clean:
	rm -f *.bin *.x86_64 *.o *.map splash.h
