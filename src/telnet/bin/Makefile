USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

PROGRAM=telnet
SOURCES := \
	$(SRC_DIR)/telnet.c \
	$(SRC_DIR)/../lib/dget_text_single.c

#Fastirq is needed in telnet

CC65SOURCES := \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/serial/c/simple_serial_printf.c \
	$(serial_hw_SOURCES) \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/dputs.s \
	$(SRC_DIR)/../lib/dputc.s \
	$(SRC_DIR)/../lib/scroll.s \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/fastirq$(iigs_suffix).s \

GCCSOURCES := \
	$(SRC_DIR)/../lib/serial/c/simple_serial.c \
	$(SRC_DIR)/../lib/serial/c/simple_serial_io.c \
	$(SRC_DIR)/../lib/surl/c/surl_core.c \
	$(SRC_DIR)/../lib/c/scrollwindow.c \
	$(SRC_DIR)/../lib/c/path_helper.c \
	$(SRC_DIR)/../lib/c/malloc0.c \
	$(SRC_DIR)/../lib/strtrim.c \
	$(SRC_DIR)/../lib/extended_conio.c \
	$(SRC_DIR)/../lib/c/clrzone.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I $(SRC_DIR)/../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
CLCFLAGS = -t $(CC65_TARGET) -Osir -Cl $(iigs_CFLAGS) -C $(SRC_DIR)/../../config/apple2enh-rtonce.cfg $(USE_LANGCARD) -DAVOID_ROM_CALLS --asm-define AVOID_ROM_CALLS
GCCCFLAGS = -g -O0 -Wall -Wno-char-subscripts -Wno-unused-function -Wno-unknown-pragmas -Werror
GCCLDFLAGS = 

.PHONY: all clean

all: telnet-$(suffix).po

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
telnet-$(suffix).po: $(PROGRAM).bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ TELNET
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ TELNET.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	for r in *.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

$(PROGRAM).bin: $(SOURCES) $(CC65SOURCES)
	cl65 --debug-info -Wl --dbgfile,"telnet.dbg" -Ln telnet.lbl $(CFLAGS) $(CLCFLAGS) -m $(PROGRAM).map -o $@ $^

$(PROGRAM).x86_64: $(SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@	

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).x86_64 *.o *.map *.dbg *.lbl *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/ && \
	cp $(PROGRAM).bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/net/
