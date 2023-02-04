PROGRAM=surl surlput
surl_SOURCES := surl-client.c ../lib/simple_serial.c ../lib/cgets.c ../lib/scrollwindow.c ../lib/surl_core.c ../lib/surl_receive.c
surlput_SOURCES := surl-upload.c ../lib/simple_serial.c ../lib/cgets.c ../lib/scrollwindow.c \
	../lib/surl_core.c ../lib/surl_send.c ../lib/surl_receive.c ../lib/get_filedetails.c
GCCSOURCES := ../lib/extended_string.c
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib
CLCFLAGS = -t $(CC65_TARGET)
GCCCFLAGS = -g -O0 -Werror 
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
