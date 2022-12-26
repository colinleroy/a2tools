PROGRAM=a2send
SOURCES := a2send.c

CFLAGS=-g -O0 -Werror

.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -o $@ $^ $(DEPS)

install:
	install $(PROGRAM) /usr/local/bin

clean:
	rm -f $(PROGRAM) *.o
