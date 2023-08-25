prefix ?= /usr/local
PROGRAM=trace-run
SOURCES := trace-run.c \
	instructions.c \
	symbols.c \
	../lib/strsplit.c \
	../../../cc65/src/dbginfo/dbginfo.c

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
