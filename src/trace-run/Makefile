prefix ?= /usr/local
PROGRAM=trace-run
SOURCES := trace-run.c ../lib/strsplit.c

CFLAGS= -I ../lib -g -O0 -Wall -Wno-deprecated-declarations
.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	gcc $(CFLAGS) $^ -o $@

clean:
	rm -f $(PROGRAM) *.o

install:
	mkdir -p $(DESTDIR)/$(prefix)/bin
	install $(PROGRAM) $(DESTDIR)/$(prefix)/bin/$(PROGRAM)
