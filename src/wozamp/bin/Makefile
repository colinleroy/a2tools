USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

wozamp_SOURCES := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/config.c \
	$(EMBED_RBROWSER) \
	$(SRC_DIR)/../lib/dget_text_single.c \
	$(SRC_DIR)/../lib/strsplit.c \
	$(SRC_DIR)/../stp/stp_list.c \
	$(SRC_DIR)/../lib/charsets.c

wozamp_CL65SOURCES = \
	$(SRC_DIR)/../lib/surl/surl_stream_audio/$(STREAMER_VER).s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_configure.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/common.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/no_irq.s \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_ping.s \
	$(SRC_DIR)/../lib/surl/asm/surl_receive.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/asm/realloc_safe.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/asm/backup_hgrpage.s \
	$(SRC_DIR)/../lib/dputc.s \
	$(SRC_DIR)/../lib/scroll.s \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/runtime_once_clean.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/platform_sleep.s \
	$(SRC_DIR)/../lib/citoa.s \
	$(SRC_DIR)/../lib/cutoa.s \
	$(SRC_DIR)/../lib/cntoabuf.s \

wozamp_CLCFLAGS = -t $(CC65_TARGET) -DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-DSTP_DATA_SIZE=8192 \
	-I $(SRC_DIR)/../lib -I $(SRC_DIR)/../stp -I . \
	-Wl -D,__STACKSIZE__=0x0400 -C $(SRC_DIR)/../../config/apple2enh-pwmstreamer.cfg \
	-vm -m wozamp.map $(USE_LANGCARD) -O

rbrowser_SOURCES := \
	$(SRC_DIR)/radio-browser.c \
	$(SRC_DIR)/config.c \
	$(SRC_DIR)/../lib/dget_text_single.c \
	$(SRC_DIR)/../lib/strsplit.c

rbrowser_CL65SOURCES = \
	$(SRC_DIR)/../lib/surl/surl_stream_audio/$(STREAMER_VER).s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/common.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/control.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/no_irq.s \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_get_json.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/asm/backup_hgrpage.s \
	$(SRC_DIR)/../lib/dputc.s \
	$(SRC_DIR)/../lib/dputs.s \
	$(SRC_DIR)/../lib/scroll.s \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/fastirq$(iigs_suffix).s \
	$(SRC_DIR)/../lib/runtime_once_clean.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/asm/clrzone.s \
	$(SRC_DIR)/../lib/cntoabuf.s \
	$(SRC_DIR)/../lib/cutoa.s \

rbrowser_CLCFLAGS = -t $(CC65_TARGET) \
	-I $(SRC_DIR)/../lib -I $(SRC_DIR)/../stp -I . \
	-Wl -D,__STACKSIZE__=0x0400 -C $(SRC_DIR)/../../config/apple2enh-rbrowser.cfg \
	-vm -m rbrowser.map $(USE_LANGCARD) -O --start-addr 0x4000


.PHONY: all clean

### Warning! some memory tricks happen.

#0x4000-0x6000 is HGR page 2,
#0x4000-0x6000 is ALSO used to store setup code in RT_ONCE in order to shrink
#              the binary a bit. But DO NOT runtime_once_clean() as usual!

all dist: wozamp-$(suffix).po
perso: all wozampperso-$(suffix).po

ifdef BUILD_VIDEOPLAY
videoplay_SOURCES := \
	$(SRC_DIR)/../lib/surl/surl_stream_av/$(STREAMER_VER).s \
	$(SRC_DIR)/videoplay.c \
	$(SRC_DIR)/../lib/FVTABZ.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial.s \
	$(SRC_DIR)/../lib/serial/asm/simple_serial_io.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/common.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/control.s \
	$(SRC_DIR)/../lib/serial/$(serial_hw)/no_irq.s \
	$(SRC_DIR)/../lib/surl/asm/surl_core.s \
	$(SRC_DIR)/../lib/surl/asm/surl_read_with_barrier.s \
	$(SRC_DIR)/../lib/asm/scrollwindow.s \
	$(SRC_DIR)/../lib/asm/path_helper.s \
	$(SRC_DIR)/../lib/asm/malloc0.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/fastirq$(iigs_suffix).s \
	$(SRC_DIR)/../lib/extrazp.s

videoplay_CLCFLAGS = -t $(CC65_TARGET) \
	-DDOUBLE_BUFFER --asm-define DOUBLE_BUFFER \
	-I $(SRC_DIR)/../lib -I . \
	-Wl -D,__STACKSIZE__=0x0400 -C $(SRC_DIR)/../../config/apple2enh-wozamp-videoplay.cfg \
	-vm -m videoplay.map $(USE_LANGCARD) -O --start-addr $(VIDEOPLAY_START_ADDR)

DISK_PROGS = splash.h splash-video.h wozamp.bin $(EXTERNAL_RBROWSER) videoplay.bin

else

DISK_PROGS = splash.h wozamp.bin $(EXTERNAL_RBROWSER)

endif

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
wozamp-$(suffix).po: $(DISK_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ WOZAMP
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ WOZAMP.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	for r in *.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

wozampperso-$(suffix).po: wozamp-$(suffix).po
	cp $< $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ STPSTARTURL TXT < $(SRC_DIR)/STPSTARTURL; \

clean:
	rm -f *.bin *.o *.map splash.h splash-video.h splash.hgr splash-video.hgr *.lbl *.dbg *.po *.hgr

wozamp.hgr: $(SRC_DIR)/wozamp.png
	$(SRC_DIR)/../surl-server/hgr-convert $<
	mv $<.hgr .

wozamp-video.hgr: $(SRC_DIR)/wozamp-video.png
	$(SRC_DIR)/../surl-server/hgr-convert $<
	mv $<.hgr .

splash.h: wozamp.hgr
	$(SRC_DIR)/../../bin/make-splash.sh wozamp.png splash.h

splash-video.h: wozamp-video.hgr
	$(SRC_DIR)/../../bin/make-splash.sh wozamp-video.png splash-video.h

wozamp.bin: $(wozamp_CL65SOURCES) $(wozamp_SOURCES)
	cl65 -g -Wl --dbgfile,wozamp.dbg -Ln wozamp.lbl $(CFLAGS) $(iigs_CFLAGS) $(wozamp_CLCFLAGS) -o $@ $^

rbrowser.bin: $(rbrowser_CL65SOURCES) $(rbrowser_SOURCES)
	cl65 -g -Wl --dbgfile,rbrowser.dbg -Ln rbrowser.lbl $(CFLAGS) $(iigs_CFLAGS) $(rbrowser_CLCFLAGS) -o $@ $^

videoplay.bin: $(videoplay_CL65SOURCES) $(videoplay_SOURCES)
	cl65 -g -Wl --dbgfile,videoplay.dbg -Ln videoplay.lbl $(CFLAGS) $(iigs_CFLAGS) $(videoplay_CLCFLAGS) -o $@ $^

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/wozamp/$(suffix)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/wozamp/$(suffix)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/wozamp/$(suffix)/

dist: wozamp-$(suffix).po
	mkdir -p $(SRC_DIR)../../dist/
	cp $< $(SRC_DIR)../../dist/
