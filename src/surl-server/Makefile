prefix ?= /usr/local
PROGRAM=surl-server
SOURCES := main.c raw-session.c char_convert.c jq.c html2txt.c hgr.c \
	../lib/extended_string.c ../lib/strsplit.c \
	../lib/simple_serial.c ../lib/clrzone.c ../lib/extended_conio.c	

CFLAGS= -I ../lib -g -O0 -Wall -DCONF_FILE_PATH=\"/etc/surl-server/tty.conf\"
.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	gcc $(CFLAGS) $(shell curl-config --cflags) $(shell libpng-config --cflags) \
		$^ \
		$(shell curl-config --libs) $(shell libpng-config --libs) \
		-ljpeg -ljq -lgumbo -o $@

clean:
	rm -f $(PROGRAM) *.o

install:
	mkdir -p $(DESTDIR)/etc/surl-server
	mkdir -p $(DESTDIR)/$(prefix)/bin
	install $(PROGRAM) $(DESTDIR)/$(prefix)/bin/$(PROGRAM)
	mkdir -p $(DESTDIR)/lib/systemd/system/
	install -m 644 systemd/surl-server.service $(DESTDIR)/lib/systemd/system/surl-server.service
