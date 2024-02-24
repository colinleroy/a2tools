prefix ?= /usr/local
PROGRAM=ba-server.x86_64
SOURCES := main.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_io.c \
	../lib/malloc0.c \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/clrzone.c

CFLAGS= -I ../lib -g -O0 -Wall -Wno-char-subscripts -Wno-deprecated-declarations -Werror -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
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
		-ljq -lgumbo -o $@

clean:
	rm -f $(PROGRAM) *.o
