PROGRAM=stp
SOURCES := stp.c stp_cli.c stp_save.c stp_send_file.c stp_delete.c \
	../lib/simple_serial.c \
	../lib/get_buf_size.c \
	../lib/extended_conio.c \
	../lib/progress_bar.c \
	../lib/strsplit.c \
	../lib/get_filedetails.c \
	../lib/dgets.c \
	../lib/clrzone.c \
	../lib/scrollwindow.c \
	../lib/surl_core.c \
	../lib/surl_receive.c \
	../lib/surl_send.c \
	../lib/surl_set_time.c

CL65SOURCES = ../lib/dputc.s ../lib/scroll.s
GCC_SOURCES = ../lib/extended_string.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -O -g --debug-info -Wl "-Ln,$(PROGRAM).lbl" -Wl "--dbgfile,$(PROGRAM).dbg" -Wl "--mapfile,$(PROGRAM).map" -vm #-DSERIAL_TO_LANGCARD
GCCCFLAGS = -g -O0 -Wall 
GCCLDFLAGS = 

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

$(PROGRAM).bin: $(SOURCES) $(CL65SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -o $@ $^ 

$(PROGRAM).x86_64: $(SOURCES) $(GCC_SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ || \
	mkdir /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/net/
