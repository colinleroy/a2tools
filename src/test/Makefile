CC65_TARGET = apple2enh
ifdef IIGS
serial_hw := z8530
iigs_suffix := -iigs
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
else
serial_hw := acia
iigs_suffix :=
iigs_CFLAGS :=
CC65_TARGET = apple2enh
trgt = apple2c0
endif

PROGRAM=test
SOURCES := \
	../lib/surl/surl_stream_av.s \
	main.c \
	../lib/hgr.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_io.s \
	../lib/serial/serial_read_no_irq.s \
	../lib/serial/$(serial_hw)/common.s \
	../lib/serial/$(serial_hw)/control.s \
	../lib/serial/$(serial_hw)/no_irq.s \
	../lib/path_helper.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_read_with_barrier.s \
	../lib/fastirq$(iigs_suffix).s \
	../lib/extrazp.s \
	../lib/malloc0.c \

CLCFLAGS = -t $(CC65_TARGET) -DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER -I ../lib \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-avstreamer.cfg \
	-vm -m $(PROGRAM).map -DSURL_TO_LANGCARD -O

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
	mame $(trgt)  -window -flop1 test.dsk -resolution 560x384 -printer null_modem -bitb socket.localhost:2001 -modem null_modem -bitb2 socket.localhost:2000 -mouse -debug

test-hd: clean all
	cp ../../disks/basic-empty.dsk ./test.dsk && \
	java -jar ../../bin/ac.jar -p test.dsk TEST.SYSTEM SYS < ../../bin/loader.system && \
	java -jar ../../bin/ac.jar -as test.dsk TEST BIN < test.bin && \
	java -jar ../../bin/ac.jar -d test.dsk BASIC.SYSTEM && \
	mame apple2gs -window -flop1 test.dsk -resolution 560x384 -printer null_modem -bitb socket.localhost:2001 -modem null_modem -bitb2 socket.localhost:2000 -mouse -debug -sound none -sl7 scsi -hard ../../disks/smart0.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/ && \
	cp test.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/
