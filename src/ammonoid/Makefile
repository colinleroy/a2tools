TARGET := apple2
CPU := 6502

GENERAL_CFLAGS := -DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_ENABLE_IRQ \
	-DDYN_BOX_DRAW -DAPPLE2_INCLUDE_IIE_CHARS \
	-DPROGRAM_NAME='"AMMONOID"' \

FLOP_NAME = AMMONOID
CLEANDISK = ../../disks/ProDOS_2_4_3.po

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -I ../stp -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror

.PHONY: all clean

ammonoid_SOURCES := \
	main.c \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/malloc0.s \
	../lib/asm/clrzone.s \
	../lib/dget_text_single.c \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s \
	../lib/vsdrive.s \
	../lib/get_buf_size.c \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/a2_features.s \

ammonoid_CLCFLAGS = -t $(TARGET) -I ../lib \
	-DFILESEL_ALLOW_NONPRODOS_VOLUMES \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-rtonce.cfg \
	-vm -m ammonoid.map $(GENERAL_CFLAGS) -O -Cl --start-addr 0x1003

all: \
	ammonoid.po

clean:
	rm -f *.bin *.o *.map splash.h splash.hgr

ammonoid.bin: $(ammonoid_CL65SOURCES) $(ammonoid_SOURCES)
	cl65 -g -Wl --dbgfile,ammonoid.dbg -Ln ammonoid.lbl $(CFLAGS) $(ammonoid_CLCFLAGS) -o $@ $^  $(TARGET)-iobuf-0800.o

../image-viewer/imgview.bin:
	make -C ../image-viewer/ clean all

add_disk:
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)AMMONOID.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -d $(IMG) $(PREFIX)BASIC.SYSTEM; \
	for r in *.bin ../image-viewer/imgview.bin; do \
		java -jar ../../bin/ac.jar -as $(IMG) $(PREFIX)$$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

ammonoid.po: ammonoid.bin ../image-viewer/imgview.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ $(FLOP_NAME)
	IMG=$@ PREFIX= make add_disk

hdv: 
	java -jar ../../bin/ac.jar -d $(IMG) $(FLOP_NAME)
	PREFIX=$(FLOP_NAME)/ make add_disk

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/ammonoid/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/ammonoid/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/ammonoid/ && \
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/

dist: ammonoid.po
	mkdir -p ../../dist/
	cp $< ../../dist/
