prefix ?= /usr/local
PROGRAM=surl-server
SOURCES := main.c raw-session.c \
	helpers/char-convert.c helpers/jq-get.c helpers/html2txt.c helpers/hgr.c \
	../lib/extended_string.c ../lib/strsplit.c \
	../lib/simple_serial.c ../lib/simple_serial_printf.c \
	../lib/clrzone.c ../lib/extended_conio.c	

hgrconv_SOURCES := hgr-convert.c helpers/hgr.c

CFLAGS= -I ../lib -I helpers $(shell sdl-config --cflags) -g -O0 -Wall -Wno-char-subscripts -Wno-deprecated-declarations -Werror -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
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

hgr-convert: $(hgrconv_SOURCES)
	gcc $(CFLAGS) \
		$(shell libpng-config --cflags) \
		$(shell pkg-config --cflags SDL_image) \
		$^ \
		$(shell libpng-config --libs) \
		$(shell pkg-config --libs SDL_image) \
		-o $@

clean:
	rm -f $(PROGRAM) *.o

install:
	mkdir -p $(DESTDIR)/etc/surl-server
	mkdir -p $(DESTDIR)/$(prefix)/bin
	install $(PROGRAM) $(DESTDIR)/$(prefix)/bin/$(PROGRAM)
	mkdir -p $(DESTDIR)/lib/systemd/system/
	install -m 644 systemd/surl-server.service $(DESTDIR)/lib/systemd/system/surl-server.service
