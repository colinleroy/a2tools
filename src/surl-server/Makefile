prefix ?= /usr/local
etc_prefix ?= /usr/local/etc

FONTS_DIR = $(prefix)/share/fonts/truetype/Apple2
MONO_FONT = "beepu-mono.ttf"
PROP_FONT = "beepu.ttf"

PROGRAM=surl-server
SOURCES := main.c raw-session.c \
	helpers/char-convert.c \
	helpers/jq-get.c \
	helpers/html2txt.c \
	helpers/hgr-convert.c \
	helpers/stream.c \
	helpers/ffmpeg.c \
	helpers/printer.c \
	helpers/vsdrive.c \
	helpers/imagewriter.cpp \
	../lib/strtrim.c ../lib/strsplit.c \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/serial/c/simple_serial_gets.c \
	../lib/serial/c/simple_serial_printf.c \
	../lib/c/malloc0.c \
	../lib/c/clrzone.c \
	../lib/extended_conio.c \
	../lib/array_sort.c \
	../lib/charsets.c
	

hgrconv_SOURCES := hgr-convert.c helpers/hgr-convert.c

CFLAGS= -I ../lib -I helpers $(shell sdl-config --cflags) \
				-g -O0 -Wall -Wno-char-subscripts -Wno-deprecated-declarations \
				-Wall \
				-DCONF_FILE_PATH=\"$(etc_prefix)/a2tools/tty.conf\" \
				-DPRINTER_CONF_FILE_PATH=\"$(etc_prefix)/a2tools/printer.conf\" \
				-DVSDRIVE_CONF_FILE_PATH=\"$(etc_prefix)/a2tools/vsdrive.conf\" \
				-DFONTS_DIR=\"$(FONTS_DIR)\" \
				-DMONO_FONT=\"$(MONO_FONT)\" \
				-DPROP_FONT=\"$(PROP_FONT)\"

.PHONY: all clean
all: $(PROGRAM) hgr-convert

CUPSFILTER_VERSION = $(shell pkg-config  --modversion libcupsfilters|cut -b1-1)
CUPSFILTER_CFLAGS  = $(shell pkg-config  --cflags libcupsfilters)
CUPSFILTER_LIBS    = $(shell pkg-config  --libs libcupsfilters)

$(PROGRAM): $(SOURCES)
	gcc $(CFLAGS) \
		$(shell curl-config --cflags) \
		$(shell cups-config --cflags) \
		$(shell libpng-config --cflags) \
		$(shell pkg-config --cflags \
						SDL_image gumbo libmagic libjq\
						libavfilter libavformat libavformat libavcodec \
						libavutil libswresample freetype2) \
		$(CUPSFILTER_CFLAGS) \
		-DCUPSFILTER_VERSION="$(CUPSFILTER_VERSION)" \
		$^ \
		$(shell curl-config --libs) \
		$(shell cups-config --libs) \
		$(shell libpng-config --libs) \
		$(shell pkg-config --libs \
						SDL_image gumbo libmagic libjq\
						libavfilter libavformat libavformat libavcodec \
						libavutil libswresample freetype2) \
		$(CUPSFILTER_LIBS) \
		-lpthread -lstdc++ -lm \
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
	rm -f $(PROGRAM) hgr-convert *.o

install:
	mkdir -p $(DESTDIR)/$(etc_prefix)/a2tools
	mkdir -p $(DESTDIR)/$(prefix)/bin
	install $(PROGRAM) $(DESTDIR)/$(prefix)/bin/$(PROGRAM)
	install hgr-convert $(DESTDIR)/$(prefix)/bin/hgr-convert
	mkdir -p $(DESTDIR)/lib/systemd/system/
	if [ "$(DESTDIR)" = "" ]; then \
		id surl-server || (adduser --system --no-create-home surl-server && adduser surl-server dialout) \
	fi
	install -m 644 systemd/surl-server.service $(DESTDIR)/lib/systemd/system/surl-server.service
	mkdir -p $(DESTDIR)/$(FONTS_DIR)
	install -m 644 fonts/$(MONO_FONT) $(DESTDIR)/$(FONTS_DIR)
	install -m 644 fonts/$(PROP_FONT) $(DESTDIR)/$(FONTS_DIR)
	fc-cache || true
