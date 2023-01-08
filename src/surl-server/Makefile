PROGRAM=surl-server
SOURCES := main.c ../lib/extended_string.c ../lib/simple_serial.c

CFLAGS= -I ../lib -g -O0 -Wall -DCONF_FILE_PATH=\"/etc/surl-server/tty.conf\"
.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	gcc $(CFLAGS) $(shell curl-config --cflags) $^ $(shell curl-config --libs) -o $@

clean:
	rm -f $(PROGRAM) *.o

install:
	install $(PROGRAM) /usr/local/bin/$(PROGRAM)
	install -m 644 systemd/surl-server.service /lib/systemd/system/surl-server.service
	systemctl daemon-reload
