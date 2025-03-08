USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

PROGRAM=homectrl grphview
homectrl_SOURCES := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/network.c \
	$(SRC_DIR)/switches.c \
	$(SRC_DIR)/sensors.c \
	$(SRC_DIR)/climate.c \
	$(SRC_DIR)/server_url.c \
	$(SRC_DIR)/../lib/extended_conio.c \
	$(SRC_DIR)/../lib/dget_text_single.c \
	$(SRC_DIR)/../lib/strsplit.c \
	$(SRC_DIR)/../lib/strellipsis.c \
	
homectrl_CL65SOURCES = \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_configure.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_ping.s \
	$(SRC_DIR)/../lib/surl/asm/surl_receive.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/dputc.s\
	$(SRC_DIR)/../lib/dputs.s \
	$(SRC_DIR)/../lib/scroll.s \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/citoa.s \
	$(SRC_DIR)/../lib/cutoa.s \
	$(SRC_DIR)/../lib/cntoabuf.s \

homectrl_GCCSOURCES = \
	$(SRC_DIR)/../lib/serial/c/simple_serial.c \
	$(SRC_DIR)/../lib/serial/c/simple_serial_io.c \
	$(SRC_DIR)/../lib/surl/c/surl_core.c \
	$(SRC_DIR)/../lib/surl/c/surl_ping.c \
	$(SRC_DIR)/../lib/surl/c/surl_receive.c \
	$(SRC_DIR)/../lib/c/scrollwindow.c \
	$(SRC_DIR)/../lib/c/malloc0.c \
	$(SRC_DIR)/../lib/c/path_helper.c \
	$(SRC_DIR)/../lib/c/clrzone.c \
	$(SRC_DIR)/../lib/strtrim.c

grphview_SOURCES := \
	$(SRC_DIR)/graph-viewer.c \
	$(SRC_DIR)/../lib/surl/surl_receive_lines.c
	
grphview_CL65SOURCES = \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(serial_hw_SOURCES) \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/fastirq$(iigs_suffix).s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/FVTABZ.s

grphview_GCCSOURCES := \
	$(SRC_DIR)/../lib/serial/c/simple_serial.c \
	$(SRC_DIR)/../lib/serial/c/simple_serial_io.c \
	$(SRC_DIR)/../lib/surl/c/surl_core.c \
	$(SRC_DIR)/../lib/c/path_helper.c \
	$(SRC_DIR)/../lib/c/clrzone.c \
	$(SRC_DIR)/../lib/c/malloc0.c \
	$(SRC_DIR)/../lib/strtrim.c \
	$(SRC_DIR)/../lib/extended_conio.c \
	$(SRC_DIR)/../lib/tgi_sdl.c \
	$(SRC_DIR)/../lib/tgi_fastline.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I $(SRC_DIR)/../lib -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\"
homectrl_CLCFLAGS = -t $(CC65_TARGET) -O -Wl -D,__STACKSIZE__=0x0400 -C $(SRC_DIR)/../../config/apple2enh-rtonce.cfg $(USE_LANGCARD) $(iigs_CFLAGS)
grphview_CLCFLAGS = $(homectrl_CLCFLAGS) --start-addr 0x4000
GCCCFLAGS = -g -O0 -Wall  -Wno-unknown-pragmas -Wno-unused-function -Wno-char-subscripts -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)
.PHONY: all clean

all: homecontrol-$(suffix).po
	
CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
homecontrol-$(suffix).po: homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ HOMECONTROL
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ HOMECTRL.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	for r in *.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

homecontrolperso-$(suffix).po: homecontrol-$(suffix).po
	cp $< $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ SRVURL TXT < $(SRC_DIR)/SRVURL; \

homectrl.bin: $(homectrl_SOURCES) $(homectrl_CL65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"homectrl.dbg" $(CFLAGS) $(homectrl_CLCFLAGS) -m homectrl.map -o $@ $^

homectrl.x86_64: $(homectrl_SOURCES) $(homectrl_GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

grphview.bin: $(grphview_SOURCES) $(grphview_CL65SOURCES)
	cl65 -g --debug-info -Wl --dbgfile,"grphview.dbg" $(CFLAGS) $(grphview_CLCFLAGS) -m grphview.map -o $@ $^

grphview.x86_64: $(grphview_SOURCES) $(grphview_GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64 *.o *.map *.dbg *.lbl *.po

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/homecontrol/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/homecontrol/ && \
	cp homectrl.bin grphview.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/homecontrol/
