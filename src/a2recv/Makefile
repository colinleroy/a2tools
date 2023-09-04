prefix ?= /usr/local
PROGRAM=a2recv
SOURCES := a2recv.c ../lib/simple_serial.c ../lib/clrzone.c
GCCSOURCES := ../lib/extended_string.c ../lib/extended_conio.c
CL65SOURCES := ../lib/fastirq.s
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -DSURL_TO_LANGCARD
GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Werror

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

install:
	mkdir -p $(DESTDIR)/$(prefix)/bin
	install $(PROGRAM).x86_64 $(DESTDIR)/$(prefix)/bin/$(PROGRAM)

$(PROGRAM).bin: $(SOURCES) $(CL65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ || \
	mkdir /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/
