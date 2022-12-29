PROGRAM=homectrl
SOURCES := main.c network.c \
	switches.c sensors.c heating.c \
	../lib/simple_serial.c \
	../lib/extended_conio.c \
	../lib/extended_string.c \
	../lib/http.c \
	../lib/slist.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib
CLCFLAGS = -t $(CC65_TARGET)
GCCCFLAGS = -g -O0 -Werror $(shell curl-config --cflags)
GCCLDFLAGS = $(shell curl-config --libs)
.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

$(PROGRAM).bin: $(SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map
