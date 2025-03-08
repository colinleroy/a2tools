PROGRAM=stp
SOURCES := \
	$(SRC_DIR)/stp_main.c \
	$(SRC_DIR)/stp_list.c \
	$(SRC_DIR)/stp_save.c \
	$(SRC_DIR)/stp_send_file.c \
	$(SRC_DIR)/stp_delete.c \
	$(SRC_DIR)/stp_create_dir.c \
	$(SRC_DIR)/../lib/get_buf_size.c \
	$(SRC_DIR)/../lib/progress_bar.c \
	$(SRC_DIR)/../lib/strsplit.c \
	$(SRC_DIR)/../lib/dget_text_single.c \
	$(SRC_DIR)/../lib/surl/surl_set_time.c

CL65SOURCES = \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_configure.s \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_ping.s \
	$(SRC_DIR)/../lib/surl/asm/surl_send.s \
	$(SRC_DIR)/../lib/surl/asm/surl_receive.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(serial_hw_SOURCES) \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/asm/realloc_safe.s \
	$(SRC_DIR)/../lib/asm/file_select.s \
	$(SRC_DIR)/../lib/dputc.s \
	$(SRC_DIR)/../lib/scroll.s \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/runtime_once_clean.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/platform_sleep.s \
	$(SRC_DIR)/../lib/get_dev_from_path.c \
	$(SRC_DIR)/../lib/citoa.s \
	$(SRC_DIR)/../lib/cutoa.s \
	$(SRC_DIR)/../lib/cntoabuf.s \
	$(SRC_DIR)/../lib/vsdrive.s \
	#$(SRC_DIR)/../lib/fastirq$(suffix).s \

GCC_SOURCES = \
	$(SRC_DIR)/../lib/serial/c/simple_serial.c \
	$(SRC_DIR)/../lib/serial/c/simple_serial_io.c \
	$(SRC_DIR)/../lib/surl/c/surl_core.c \
	$(SRC_DIR)/../lib/surl/c/surl_ping.c \
	$(SRC_DIR)/../lib/surl/c/surl_send.c \
	$(SRC_DIR)/../lib/surl/c/surl_receive.c \
	$(SRC_DIR)/../lib/c/scrollwindow.c \
	$(SRC_DIR)/../lib/c/path_helper.c \
	$(SRC_DIR)/../lib/c/malloc0.c \
	$(SRC_DIR)/../lib/c/realloc_safe.c \
	$(SRC_DIR)/../lib/c/file_select.c \
	$(SRC_DIR)/../lib/strtrim.c \
	$(SRC_DIR)/../lib/extended_conio.c \
	$(SRC_DIR)/../lib/c/clrzone.c

DEPS := 

CC65_TARGET = $(TARGET)
CFLAGS  = -I $(SRC_DIR)/../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" $(debug_CFLAGS)
CLCFLAGS = -t $(CC65_TARGET) -g -O -Cl -Wl -D,__STACKSIZE__=0x0400 \
	-C $(SRC_DIR)/../../config/apple2enh-rtonce.cfg -vm -m $(PROGRAM).map \
	-DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD -DFILESEL_TO_LANGCARD \
	-DFILESEL_ALLOW_NONPRODOS_VOLUMES --asm-define FILESEL_ALLOW_NONPRODOS_VOLUMES \
	$(iigs_CFLAGS) -DSTP_DATA_SIZE=8192
GCCCFLAGS = -g -O0 -Wall -Wno-format -Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas -Werror -DSTP_DATA_SIZE=8192
GCCLDFLAGS = 

.PHONY: all clean

all: stp-$(suffix).po stpperso-$(suffix).po

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
stp-$(suffix).po: $(PROGRAM).bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ STP
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ STP.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	for r in *.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

stpperso-$(suffix).po: $(PROGRAM).bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ STP
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ STP.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ STPSTARTURL TXT < $(SRC_DIR)/STPSTARTURL; \
	for r in *.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

$(PROGRAM).bin: $(SOURCES) $(CL65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"stp.dbg" $(CFLAGS) $(CLCFLAGS) -o $@ $^ -Ln stp.lbl

$(PROGRAM).x86_64: $(SOURCES) $(GCC_SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map *.dbg *.lbl *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/
