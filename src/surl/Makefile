PROGRAM=surl surlput
surl_SOURCES := surl-client.c ../lib/simple_serial.c ../lib/cgets.c ../lib/scrollwindow.c ../lib/surl_core.c ../lib/surl_receive.c
surlput_SOURCES := surl-upload.c ../lib/simple_serial.c ../lib/cgets.c ../lib/scrollwindow.c \
	../lib/surl_core.c ../lib/surl_send.c ../lib/surl_receive.c ../lib/get_filedetails.c
GCCSOURCES := ../lib/extended_string.c
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET)
GCCCFLAGS = -g -O0 -Wall
GCCLDFLAGS = 

.PHONY: all clean

all: surl.bin surl.x86_64 surlput.bin surlput.x86_64

surl.bin: $(surl_SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m surl.map -o $@ $^

surl.x86_64: $(surl_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

surlput.bin: $(surlput_SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m surl.map -o $@ $^

surlput.x86_64: $(surlput_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f surl.bin surl.x86_64 surlput.bin surlput.x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ || \
	mkdir /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ && \
	cp surl.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/
