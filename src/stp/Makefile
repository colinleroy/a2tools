TARGET := apple2

GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	-DFILESEL_TO_LANGCARD \
	-DFILESEL_ALLOW_NONPRODOS_VOLUMES \
	--asm-define FILESEL_ALLOW_NONPRODOS_VOLUMES \
	--asm-define SERIAL_ENABLE_IRQ \
	-DSTP_DATA_SIZE=8192 \
	-DDYN_BOX_DRAW -DAPPLE2_INCLUDE_IIE_CHARS \

PROGRAM=stp
SOURCES := \
	stp_main.c \
	stp_list.c \
	stp_save.c \
	stp_send_file.c \
	stp_delete.c \
	stp_create_dir.c \
	../lib/get_buf_size.c \
	../lib/progress_bar.c \
	../lib/strsplit.c \
	../lib/dget_text_single.c \
	../lib/surl/surl_set_time.c

CL65SOURCES = \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/surl/asm/surl_core.s \
	../lib/surl/asm/surl_ping.s \
	../lib/surl/asm/surl_send.s \
	../lib/surl/asm/surl_receive.s \
	../lib/surl/asm/surl_read_with_barrier.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/clrzone.s \
	../lib/asm/malloc0.s \
	../lib/asm/realloc_safe.s \
	../lib/asm/file_select.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/runtime_once_clean.s \
	../lib/extrazp.s \
	../lib/platform_sleep.s \
	../lib/get_dev_from_path.c \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s \
	../lib/vsdrive.s \
	../lib/a2_features.s \

GCC_SOURCES = \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/surl/c/surl_core.c \
	../lib/surl/c/surl_ping.c \
	../lib/surl/c/surl_send.c \
	../lib/surl/c/surl_receive.c \
	../lib/c/scrollwindow.c \
	../lib/c/path_helper.c \
	../lib/c/malloc0.c \
	../lib/c/realloc_safe.c \
	../lib/c/file_select.c \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/c/clrzone.c

DEPS := 

CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" $(debug_CFLAGS)
CLCFLAGS = -t $(TARGET) -g -O -Cl -Wl -D,__STACKSIZE__=0x0400 \
	-C ../../config/apple2enh-rtonce.cfg -vm -m $(PROGRAM).map \
	$(GENERAL_CFLAGS) \

GCCCFLAGS = -g -O0 -Wall -Wno-format -Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas -Werror -DSTP_DATA_SIZE=8192
GCCLDFLAGS = 

.PHONY: all clean

all dist: stp.po

perso: all stpperso.po

CLEANDISK = ../../disks/ProDOS_2_4_3.po
stp.po: $(PROGRAM).bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ STP
	java -jar ../../bin/ac.jar -p $@ STP.SYSTEM SYS < ../../bin/loader.system; \
	for r in *.bin; do \
		java -jar ../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

stpperso.po: stp.po
	cp $< $@; \
	java -jar ../../bin/ac.jar -p $@ STPSTARTURL TXT < STPSTARTURL; \

$(PROGRAM).bin: $(SOURCES) $(CL65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"stp.dbg" $(CFLAGS) $(CLCFLAGS) -o $@ $^ -Ln stp.lbl

$(PROGRAM).x86_64: $(SOURCES) $(GCC_SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map *.dbg *.lbl *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/$(suffix)/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/$(suffix)/

dist: stp.po
	mkdir -p ../../dist/
	cp $< ../../dist/
