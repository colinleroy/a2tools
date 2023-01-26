mastodon_SOURCES := cli/login.c api/oauth.c cli/logo.c \
	../lib/simple_serial.c ../lib/extended_conio.c \
	../lib/extended_string.c ../lib/surl.c

mastocli_SOURCES := cli/tl.c api/api.c \
	api/account.c api/status.c api/common.c \
	cli/header.c \
	../lib/simple_serial.c ../lib/extended_conio.c \
	../lib/extended_string.c ../lib/surl.c

mastowrite_SOURCES := cli/compose.c cli/compose_header.c \
	api/compose.c api/account.c api/status.c api/common.c \
	../lib/simple_serial.c ../lib/extended_conio.c \
	../lib/dgets.c \
	../lib/extended_string.c ../lib/surl.c

mastoconf_SOURCES := cli/config.c cli/logo.c ../lib/extended_conio.c

CC65SOURCES := ../lib/dputs.s ../lib/dputc.s ../lib/scroll.s
GCCSOURCES := 
DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -I api -I cli
CLCFLAGS = -t $(CC65_TARGET) -Osir -Cl -Wl -D,__STACKSIZE__=0x0400 -DSERIAL_TO_LANGCARD
GCCCFLAGS = -g -O0 -Werror 
GCCLDFLAGS = 

.PHONY: all clean

all: mastodon.bin mastodon.x86_64 \
	mastocli.bin mastocli.x86_64 \
	mastowrite.bin mastowrite.x86_64 \
	mastoconf.bin mastoconf.x86_64

mastodon.bin: $(mastodon_SOURCES) $(CC65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m mastodon.map -o $@ $^
mastocli.bin: $(mastocli_SOURCES) $(CC65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m mastocli.map -o $@ $^
mastowrite.bin: $(mastowrite_SOURCES) $(CC65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m mastowrite.map -o $@ $^
mastoconf.bin: $(mastoconf_SOURCES) $(CC65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m mastoconf.map -o $@ $^

mastodon.x86_64: $(mastodon_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastocli.x86_64: $(mastocli_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastowrite.x86_64: $(mastowrite_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	
mastoconf.x86_64: $(mastoconf_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f *.bin *.x86_64 *.o *.map

upload: all
	scp masto*.bin diskstation.lan:/volume1/a2repo/
