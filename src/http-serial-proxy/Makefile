PROGRAM=http_serial_proxy
SOURCES := main.c ../lib/extended_string.c ../lib/simple_serial.c

CFLAGS= -I ../lib -g -O0 -Wall
.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	gcc $(CFLAGS) $(shell curl-config --cflags) $^ $(shell curl-config --libs) -o $@


clean:
	rm -f $(PROGRAM) *.o
