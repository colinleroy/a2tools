GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror -DSTP_DATA_SIZE=8192

.PHONY: all clean

test_SOURCES := \
	test.c plot.s ../lib/platform_sleep.s ../lib/asm/hgr_tables.s ../lib/asm/hgr.s \

test_CLCFLAGS = -t apple2 -I ../lib \
	-Wl -D,__STACKSIZE__=0x0400 -C ../../config/apple2enh.cfg \
	-vm -m test.map $(USE_LANGCARD) -O --start-addr 0x6000 \
	--asm-define INLINE_PLOT

all: test.po

CLEANDISK = ../../disks/ProDOS_2_4_3.po
test.po: test.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ TEST
	java -jar ../../bin/ac.jar -p $@ TEST.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -p $@ A2.STDMOU.MOU BIN < /share/cc65/target/apple2/drv/mou/a2.stdmou.mou; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done; \

clean:
	rm -f *.bin *.o *.map *.dbg *.lbl *.po

test.bin: $(test_CL65SOURCES) $(test_SOURCES)
	cl65 -g -Wl --dbgfile,test.dbg -Ln test.lbl $(CFLAGS) $(test_CLCFLAGS) -o $@ $^

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/$(suffix)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/test/$(suffix)/

dist: test.po
	mkdir -p ../../dist/
	cp $< ../../dist/

test: all
	mame apple2c -flop1 test.po
