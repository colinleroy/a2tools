prefix ?= /usr/local
PROGRAM=a2trace
SOURCES := main.c \
	instructions.c \
	symbols.c \
	mame-params.c \
	../lib/strsplit.c

CFLAGS= -I ../lib -g -O0 -Wall -Wno-deprecated-declarations
.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	gcc $(CFLAGS) $^ -o $@

clean:
	rm -f $(PROGRAM) *.o

test: all
	./trace-run -d ref/in/prog.dbg -l ref/in/prog.lbl -t ref/in/prog.run > /tmp/prog.dasm && \
	./trace-run -d ref/in/prog.dbg -l ref/in/prog.lbl -t ref/in/prog.run -p > /tmp/prog.callgrind && \
	diff -u ref/out/prog.dasm /tmp/prog.dasm && \
	diff -u ref/out/prog.callgrind /tmp/prog.callgrind

install:
	mkdir -p $(DESTDIR)/$(prefix)/bin
	install $(PROGRAM) $(DESTDIR)/$(prefix)/bin/$(PROGRAM)

upload:
	true

dist:
	true
