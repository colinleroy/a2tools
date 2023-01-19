PROGRAM=stp
SOURCES := stp.c stp_cli.c stp_save.c stp_send_file.c stp_delete.c \
	../lib/simple_serial.c \
	../lib/get_buf_size.c \
	../lib/extended_conio.c \
	../lib/extended_string.c \
	../lib/get_filedetails.c \
	../lib/surl.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib
CLCFLAGS = -t $(CC65_TARGET) -Osir -g --debug-info -Wl "-Ln,$(PROGRAM).lbl" -Wl "--dbgfile,$(PROGRAM).dbg" -Wl "--mapfile,$(PROGRAM).map" -vm  #-DSERIAL_TO_LANGCARD
GCCCFLAGS = -g -O0 -Werror 
GCCLDFLAGS = 

.PHONY: all clean

all: $(PROGRAM).bin $(PROGRAM).x86_64

$(PROGRAM).bin: $(SOURCES)
	cl65 $(CFLAGS) $(CLCFLAGS) -o $@ $^ 

$(PROGRAM).x86_64: $(SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map

upload: all
	scp $(PROGRAM).bin diskstation.lan:/volume1/a2repo/
