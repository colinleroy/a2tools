CFLAGS=-I ../lib

test: *bin
	sim65 test_strsplit.bin

all: \
	test_strsplit.bin

test_strsplit_SRC:= test_strsplit.c \
	../lib/strsplit.c
test_strsplit.bin: $(test_strsplit_SRC)
	cl65 -t sim6502 $(CFLAGS) -o $@ $^


clean:
	rm -f *.bin
