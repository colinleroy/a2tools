PROGRAM=ramtest.bin
SOURCES := ramtest.c
DEPS :=

CC65_TARGET = apple2enh
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) -I ../lib -C ../../config/apple2enh-hgr.cfg
LDFLAGS = -t $(CC65_TARGET) -m $(PROGRAM).map

.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	cl65 $(CFLAGS) -m $(PROGRAM).map -o $@ $^

clean:
	rm -f $(PROGRAM) *.o *.map
