TARGET := apple2
CPU := 6502
FLOP_NAME = VBLTEST
CLEANDISK = ../../disks/ProDOS_2_4_3.po

.PHONY: all clean

vbltest_SOURCES := \
	main.c \
	irq.s

vbltest_CLCFLAGS = -t $(TARGET) -I ../lib \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh.cfg \
	-vm -m vbltest.map $(GENERAL_CFLAGS) -O -Cl --start-addr 0x6000

all: \
	vbl-test.po

clean:
	rm -f *.bin *.o *.map splash.h splash.hgr

vbltest.bin: $(vbltest_SOURCES)
	cl65 -g -Wl --dbgfile,vbltest.dbg -Ln vbltest.lbl $(CFLAGS) $(vbltest_CLCFLAGS) -o $@ $^  $(TARGET)-iobuf-0800.o

add_disk:
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)vbltest.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -d $(IMG) $(PREFIX)BASIC.SYSTEM; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $(IMG) $(PREFIX)$$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

vbl-test.po: vbltest.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ $(FLOP_NAME)
	IMG=$@ PREFIX= make add_disk

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/burgerdisk_utils/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/burgerdisk_utils/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/burgerdisk_utils/ && \
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/

dist: vbl-test.po
	mkdir -p ../../dist/
	cp $< ../../dist/
