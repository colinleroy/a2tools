ifdef IIGS
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs
else
iigs_FLAGS :=
suffix :=
upload_subdir :=
endif

prefix ?= /usr/local
PROGRAM=a2send
SOURCES := a2send.c ../lib/simple_serial.c ../lib/simple_serial_ext.c ../lib/cgets.c  ../lib/scrollwindow.c ../lib/get_filedetails.c
CL65SOURCES := ../lib/fastirq$(suffix).s
GCCSOURCES := ../lib/extended_string.c

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET)  $(iigs_CFLAGS)
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
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/
