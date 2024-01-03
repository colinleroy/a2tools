ifdef IIGS
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs
else
iigs_FLAGS :=
suffix :=
upload_subdir :=
endif

PROGRAM=surl
surl_SOURCES := surl-client.c ../lib/malloc0.c ../lib/simple_serial.c ../lib/cgets.c ../lib/scrollwindow.c ../lib/surl_core.c ../lib/surl_receive.c ../lib/path_helper.c
GCCSOURCES := ../lib/extended_string.c
CL65SOURCES := ../lib/fastirq$(suffix).s ../lib/extrazp.s
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -C ../../config/apple2enh-rtonce.cfg $(iigs_CFLAGS)
GCCCFLAGS = -g -O0 -Wall -Wno-format -Wno-unknown-pragmas -Werror
GCCLDFLAGS = 

.PHONY: all clean

all: surl.bin surl.x86_64

surl.bin: $(surl_SOURCES) $(CL65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m surl.map -o $@ $^

surl.x86_64: $(surl_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f surl.bin surl.x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ && \
	cp surl.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/
