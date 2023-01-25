mastodon_SOURCES := main.c oauth.c \
	../lib/simple_serial.c ../lib/extended_conio.c \
	../lib/extended_string.c ../lib/surl.c

mastocli_SOURCES := cli.c api.c \
	account.c status.c \
	../lib/simple_serial.c ../lib/extended_conio.c \
	../lib/dgets.c \
	../lib/extended_string.c ../lib/surl.c

CC65SOURCES := ../lib/dputs.s ../lib/dputc.s ../lib/scroll.s
GCCSOURCES := ../lib/extended_string.c
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib
CLCFLAGS = -t $(CC65_TARGET) -Osir -Cl -Wl -D,__STACKSIZE__=0x0400 -DSERIAL_TO_LANGCARD
GCCCFLAGS = -g -O0 -Werror 
GCCLDFLAGS = 

.PHONY: all clean

all: mastodon.bin mastodon.x86_64 mastocli.bin mastocli.x86_64

mastodon.bin: $(mastodon_SOURCES) $(CC65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m mastodon.map -o $@ $^
mastocli.bin: $(mastocli_SOURCES) $(CC65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m mastocli.map -o $@ $^

mastodon.x86_64: $(mastodon_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastocli.x86_64: $(mastocli_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f *.bin *.x86_64 *.o *.map

upload: all
	scp masto*.bin diskstation.lan:/volume1/a2repo/
