prefix ?= /usr/local
PROGRAM=surl-server
SOURCES := main.c raw-session.c \
	helpers/char-convert.c \
	helpers/jq-get.c \
	helpers/html2txt.c \
	helpers/hgr-convert.c \
	helpers/stream.c \
	helpers/ffmpeg_to_hgr.c \
	../lib/strtrim.c ../lib/strsplit.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_gets.c \
	../lib/serial/simple_serial_printf.c \
	../lib/serial/simple_serial_io.c \
	../lib/malloc0.c 

hgrconv_SOURCES := hgr-convert.c helpers/hgr-convert.c

CFLAGS= -I ../lib -I helpers $(shell sdl-config --cflags) \
				-g -O0 -Wall -Wno-char-subscripts -Wno-deprecated-declarations \
				-Wall -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"

.PHONY: all clean
all: $(PROGRAM) hgr-convert

$(PROGRAM): $(SOURCES)
	gcc $(CFLAGS) \
		$(shell curl-config --cflags) \
		$(shell libpng-config --cflags) \
		$(shell pkg-config --cflags SDL_image) \
		$^ \
		$(shell curl-config --libs) \
		$(shell libpng-config --libs) \
		$(shell pkg-config --libs SDL_image) \
		-ljq -lgumbo \
		-lavfilter -lavformat -lavcodec -lavutil -lswresample \
		-o $@

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
	install hgr-convert $(DESTDIR)/$(prefix)/bin/hgr-convert
	mkdir -p $(DESTDIR)/lib/systemd/system/
	if [ "$(DESTDIR)" = "" ]; then \
		id surl-server || (adduser --system --no-create-home surl-server && adduser surl-server dialout) \
	fi
	install -m 644 systemd/surl-server.service $(DESTDIR)/lib/systemd/system/surl-server.service
