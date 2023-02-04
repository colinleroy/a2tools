PROGRAM=stp
SOURCES := stp.c stp_cli.c stp_save.c stp_send_file.c stp_delete.c \
	../lib/simple_serial.c \
	../lib/get_buf_size.c \
	../lib/extended_conio.c \
	../lib/strsplit.c \
	../lib/get_filedetails.c \
	../lib/dgets.c \
	../lib/clrzone.c \
	../lib/scrollwindow.c \
	../lib/surl_core.c \
	../lib/surl_receive.c \
	../lib/surl_send.c

CL65SOURCES = ../lib/dputc.s ../lib/scroll.s
GCC_SOURCES = ../lib/extended_string.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib
CLCFLAGS = -t $(CC65_TARGET) -O -g --debug-info -Wl "-Ln,$(PROGRAM).lbl" -Wl "--dbgfile,$(PROGRAM).dbg" -Wl "--mapfile,$(PROGRAM).map" -vm #-DSERIAL_TO_LANGCARD
GCCCFLAGS = -g -O0 -Werror 
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
	scp $(PROGRAM).bin diskstation.lan:/volume1/a2repo/
