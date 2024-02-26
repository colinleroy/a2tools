CC65_TARGET = apple2enh
ifdef IIGS
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
else
iigs_CFLAGS :=
CC65_TARGET = apple2enh
trgt = apple2c0
endif

PROGRAM=test
SOURCES := main.c \
	../lib/hgr.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_io.s \
	../lib/serial/serial_read_no_irq.s \
	../lib/serial/acia/common.s \
	../lib/serial/acia/control.s \
	../lib/serial/acia/no_irq.s \
	../lib/path_helper.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_read_with_barrier.s \
	../lib/surl/surl_stream.s \
	../lib/malloc0.c

CLCFLAGS = -t $(CC65_TARGET) --asm-define DOUBLE_BUFFER -I ../lib -Wl -D,__STACKSIZE__=0x0400 --start-addr 0x6000 -C ../../config/apple2enh-rtonce.cfg -vm -m $(PROGRAM).map -DSURL_TO_LANGCARD -O

.PHONY: all clean

all: $(PROGRAM).bin

$(PROGRAM).bin: $(SOURCES) $(CL65SOURCES)
	cl65 -g -Wl --dbgfile,test.dbg -Ln test.lbl $(CFLAGS) $(iigs_CFLAGS) $(CLCFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAM).bin *.o *.map

test: clean all
	cp ../../disks/basic-empty.dsk ./test.dsk && \
	java -jar ../../bin/ac.jar -p test.dsk TEST.SYSTEM SYS < ../../bin/loader.system && \
	java -jar ../../bin/ac.jar -as test.dsk TEST BIN < test.bin && \
	java -jar ../../bin/ac.jar -d test.dsk BASIC.SYSTEM && \
	mame $(trgt)  -window -flop1 test.dsk -resolution 560x384 -modem null_modem -bitb socket.localhost:2000 -mouse -debug

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/ && \
	cp test.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/
