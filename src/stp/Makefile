ifdef IIGS
serial_hw_SOURCES := ../lib/serial/z8530/common.s ../lib/serial/z8530/no_irq.s
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := -iigs
upload_subdir := /iigs
CC65_TARGET := apple2enh
else
ifdef OLDII
CC65_TARGET := apple2
else
CC65_TARGET = apple2enh
endif
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
iigs_FLAGS :=
suffix :=
upload_subdir :=
endif

PROGRAM=stp
SOURCES := stp_main.c stp_list.c stp_save.c stp_send_file.c stp_delete.c \
	../lib/malloc0.c \
	../lib/realloc_safe.c \
	../lib/serial/simple_serial.c \
	../lib/serial/simple_serial_configure.c \
	../lib/get_buf_size.c \
	../lib/progress_bar.c \
	../lib/strsplit.c \
	../lib/dgets.c \
	../lib/scrollwindow.c \
	../lib/surl/surl_core.c \
	../lib/surl/surl_receive.c \
	../lib/surl/surl_send.c \
	../lib/surl/surl_set_time.c \
	../lib/surl/surl_ping.c \
	../lib/file_select.c \
	../lib/path_helper.c \

CL65SOURCES = \
	../lib/surl/surl_read_with_barrier.s \
	../lib/serial/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/fastirq$(suffix).s \
	../lib/runtime_once_clean.s \
	../lib/extrazp.s \
	../lib/clrzone.s \
	../lib/platform_sleep.s \
	../lib/get_dev_from_path.c \
	../lib/citoa.s \
	../lib/cutoa.s \
	../lib/cntoabuf.s

GCC_SOURCES = \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/clrzone.c \
	../lib/serial/simple_serial_io.c

DEPS := 

CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" $(debug_CFLAGS)
CLCFLAGS = -t $(CC65_TARGET) -g -O -Cl -Wl -D,__STACKSIZE__=0x0400 \
	-C ../../config/apple2enh-rtonce.cfg -vm -m $(PROGRAM).map \
	-DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD -DFILESEL_TO_LANGCARD \
	-DFILESEL_ALLOW_NONPRODOS_VOLUMES \
	$(iigs_CFLAGS) -DSTP_DATA_SIZE=8192
GCCCFLAGS = -g -O0 -Wall -Wno-format -Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas -Werror -DSTP_DATA_SIZE=8192
GCCLDFLAGS = 

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

$(PROGRAM).bin: $(SOURCES) $(CL65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"stp.dbg" $(CFLAGS) $(CLCFLAGS) -o $@ $^ -Ln stp.lbl

$(PROGRAM).x86_64: $(SOURCES) $(GCC_SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/
