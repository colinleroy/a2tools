TARGET := apple2
CPU := 6502

GENERAL_CFLAGS := \
	-DDYN_BOX_DRAW -DAPPLE2_INCLUDE_IIE_CHARS \
	-DPROGRAM_NAME='"BDUTILS"' \

FLOP_NAME = BDISKUTILS
CLEANDISK = ../../disks/ProDOS_2_4_3.po

GCCCFLAGS = -g -O0 -I ../lib -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror

.PHONY: all clean

bdutils_SOURCES := \
	main.c \
	../lib/asm/smartport.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/scrollwindow.s \
	../lib/dget_text_single.c \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s \
	../lib/get_buf_size.c \
	../lib/a2_features.s \

bdutils_CLCFLAGS = -t $(TARGET) -I ../lib \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh-rtonce.cfg \
	-vm -m bdutils.map $(GENERAL_CFLAGS) -O -Cl --start-addr 0x1003

all: \
	burgerdisk_utils.po

clean:
	rm -f *.bin *.o *.map splash.h splash.hgr

bdutils.bin: $(bdutils_CL65SOURCES) $(bdutils_SOURCES)
	cl65 -g -Wl --dbgfile,bdutils.dbg -Ln bdutils.lbl $(CFLAGS) $(bdutils_CLCFLAGS) -o $@ $^  $(TARGET)-iobuf-0800.o

../image-viewer/imgview.bin:
	make -C ../image-viewer/ clean all

add_disk:
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)BDUTILS.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -d $(IMG) $(PREFIX)BASIC.SYSTEM; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $(IMG) $(PREFIX)$$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

burgerdisk_utils.po: bdutils.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ $(FLOP_NAME)
	IMG=$@ PREFIX= make add_disk

hdv: 
	java -jar ../../bin/ac.jar -d $(IMG) $(FLOP_NAME)
	PREFIX=$(FLOP_NAME)/ make add_disk

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/burgerdisk_utils/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/burgerdisk_utils/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/burgerdisk_utils/ && \
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/

dist: burgerdisk_utils.po
	mkdir -p ../../dist/
	cp $< ../../dist/
