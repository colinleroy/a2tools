prefix ?= /usr/local
PROGRAM=a2recv
SOURCES := a2recv.c ../lib/simple_serial.c ../lib/clrzone.c
GCCSOURCES := ../lib/extended_string.c ../lib/extended_conio.c
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -DSERIAL_TO_LANGCARD
GCCCFLAGS = -g -O0 -Wall

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

install:
	mkdir -p $(DESTDIR)/$(prefix)/bin
	install $(PROGRAM).x86_64 $(DESTDIR)/$(prefix)/bin/$(PROGRAM)

$(PROGRAM).bin: $(SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map

upload: all
	scp $(PROGRAM).bin diskstation.lan:/volume1/a2repo/
