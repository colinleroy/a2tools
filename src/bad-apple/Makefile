prefix ?= /usr/local
PROGRAM=ba-server.x86_64
SOURCES := main.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_io.c \
	../lib/malloc0.c \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/clrzone.c \
	../surl-server/helpers/hgr-convert.c \
	../surl-server/helpers/stream.c \
	../surl-server/helpers/ffmpeg.c \
	../lib/array_sort.c

CFLAGS= -I ../lib -I ../surl-server/helpers -g -O3 -Wall \
				-Wno-char-subscripts -Wno-deprecated-declarations -Wall -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	gcc $(CFLAGS) \
		$(shell curl-config --cflags) \
		$(shell libpng-config --cflags) \
		$(shell pkg-config --cflags SDL_image) \
		$^ \
		$(shell curl-config --libs) \
		$(shell libpng-config --libs) \
		$(shell pkg-config --libs SDL_image) \
		-ljq -lgumbo -lavfilter -lavformat -lavcodec -lavutil -lswresample -o $@

clean:
	rm -f $(PROGRAM) *.o
