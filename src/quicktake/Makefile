TARGET := apple2

GENERAL_CFLAGS := \
	-DSURL_TO_LANGCARD \
	--asm-define SURL_TO_LANGCARD \
	--asm-define SERIAL_LOW_LEVEL_CONTROL \
	--asm-define SERIAL_AUTO_DATA_BAUDRATE \
	-DDYN_BOX_DRAW -DAPPLE2_INCLUDE_IIE_CHARS \

FLOP_NAME = QUICKTAKE
CLEANDISK = ../../disks/ProDOS_2_4_3.po

ifdef DEBUG_HD
debug_CFLAGS := $(debug_CFLAGS) -DDEBUG_HD=$(DEBUG_HD) --asm-define DEBUG_HD=1
endif

ifdef DEBUG_FLOPPY
debug_CFLAGS := $(debug_CFLAGS) -DDEBUG_FLOPPY=$(DEBUG_FLOPPY)
endif

ifdef DEBUG_THUMB
debug_CFLAGS := $(debug_CFLAGS) -DDEBUG_THUMB=$(DEBUG_THUMB)
endif

VERSION := $(shell grep define.VERSION ../surl-server/surl_protocol.h |cut -d'"' -f2)

microserial_SOURCES := \
	tools/micro-serial.c \
	cameras/qt-serial.c \
	cameras/qt1x0/qt1x0-serial.c \
	cameras/qt1x0/qt1x0-thumbs.c \
	cameras/fuji/fuji-serial.c \
	cameras/fuji/fuji-read-response.c \
	cameras/fuji/fuji-thumbs.c \
	cameras/dc50/dc50-serial.c \
	cameras/dc50/dc50-read-response.c \
	cameras/dc50/dc50-thumbs.c \
	cameras/sierra/sierra-serial.c \
	cameras/sierra/sierra-read-packet.c \
	cameras/sierra/sierra-thumbs.c \
	ui/ui.c \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/serial/c/simple_serial_read_no_irq.c \
	../lib/serial/c/serial_control.c \
	../lib/c/malloc0.c \
	../lib/progress_bar.c \
	../lib/strtrim.c \

slowtake_SOURCES := \
	ui/qt-menu.c \
	ui/ui.c \
	ui/qt-edit-image.c \
	ui/qt-state.c \
	cameras/qt-serial.c \
	cameras/qt-callbacks.s \
	cameras/qt1x0/qt1x0-serial.c \
	cameras/fuji/fuji-serial.c \
	cameras/fuji/fuji-read-response.s \
	cameras/dc50/dc50-serial.c \
	cameras/dc50/dc50-read-response.c \
	cameras/sierra/sierra-serial.c \
	cameras/sierra/sierra-read-packet.s \
	../lib/dget_text_single.c \
	../lib/progress_bar.c \
	../lib/hgr_addrs.c

slowtake_CC65SOURCES := \
	ui/qt-dither-common.s \
	ui/qt-dither-horiz.s \
	ui/qt-dither-vert.s \
	cameras/qt1x0/qt1x0-thumbs.s \
	cameras/fuji/fuji-thumbs.s \
	cameras/dc50/dc50-thumbs.s \
	cameras/sierra/sierra-thumbs.s \
	../lib/serial/asm/driver.s \
	../lib/serial/asm/acia.s \
	../lib/serial/asm/z8530.s \
	../lib/serial/asm/simple_serial.s \
	../lib/serial/asm/simple_serial_io.s \
	../lib/serial/asm/simple_serial_configure.s \
	../lib/serial/asm/simple_serial_read_no_irq.s \
	../lib/a2_features.s \
	../lib/asm/zx02_decompress_in_place.s \
	../lib/asm/scrollwindow.s \
	../lib/asm/path_helper.s \
	../lib/asm/malloc0.s \
	../lib/asm/hgr.s \
	../lib/asm/reserve_auxhgr.s \
	../lib/asm/auxhgr_file.s \
	../lib/asm/unlink_auxhgr.s \
	../lib/asm/backup_screen_holes.s \
	../lib/asm/file_select.s \
	../lib/dputc.s \
	../lib/scroll.s \
	../lib/FVTABZ.s \
	../lib/asm/clrzone.s \
	../lib/platform_sleep.s \
	../lib/extrazp.s \
	../lib/runtime_once_clean.s \
	../lib/a2_features.s \

slowtake_GCCSOURCES := \
	cameras/qt1x0/qt1x0-thumbs.c \
	cameras/fuji/fuji-thumbs.c \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/serial/c/serial_control.c \
	../lib/c/scrollwindow.c \
	../lib/c/malloc0.c \
	../lib/c/hgr.c \
	../lib/c/file_select.c \
	../lib/tgi_sdl.c \
	../lib/tgi_fastline.c

qktkconv_SOURCES := \
	decoders/qt-conv.c \
	decoders/qktk/qktk_setup_decode.c \
	../lib/progress_bar.c

qktkconv_CC65SOURCES := \
	decoders/qktk/qktk.s \
	decoders/scaler.s \
	../lib/asm/path_helper.s \
	../lib/extrazp.s \
	../lib/a2_features.s \
	../lib/check_floppy.s

qktkconv_GCCSOURCES := \
	decoders/qktk/qktk.c \
	decoders/scaler.c

radcconv_SOURCES := \
	decoders/qt-conv.c \
	decoders/radc/radc_setup_decode.c \
	decoders/radc/radc.c \
	../lib/progress_bar.c

radcconv_CC65SOURCES := \
	../lib/square_table.s \
	decoders/radc/radc_bithuff.s \
	decoders/radc/radc_platform.s \
	decoders/scaler.s \
	../lib/extrazp.s \
	../lib/asm/path_helper.s \
	../lib/mult16x16mid16.s \
	../lib/approxdiv16x8.s \
	../lib/a2_features.s \
	../lib/check_floppy.s
	
radcconv_GCCSOURCES := \
	decoders/radc/radc_bithuff.c \
	decoders/radc/radc_platform.c \
	decoders/scaler.c

jpegconv_SOURCES := \
	decoders/qt-conv.c \
	decoders/jpeg/jpeg_setup_decode.c \
	decoders/jpeg/jpeg.c \
	../lib/progress_bar.c

jpegconv_CC65SOURCES := \
	../lib/square_table.s \
	decoders/jpeg/jpeg_arrays.s \
	decoders/jpeg/jpeg_platform.s \
	decoders/scaler.s \
	../lib/asm/path_helper.s \
	../lib/extrazp.s \
	../lib/a2_features.s \
	../lib/check_floppy.s

jpegconv_GCCSOURCES := \
	decoders/jpeg/jpeg_platform.c \
	decoders/scaler.c

GCCSOURCES := \
	../lib/extended_conio.c \
	../lib/c/path_helper.c \
	../lib/c/clrzone.c

DEPS :=

CFLAGS  = -I ../lib -I . -DCONF_FILE_PATH=\"/etc/a2tools/quicktake.conf\"

CLCFLAGS = -t $(TARGET) -O -Cl -Wl -D,__STACKSIZE__=0x200 $(GENERAL_CFLAGS) $(debug_CFLAGS) \

microserial_GCCCFLAGS = \
	-DBUFFER_SIZE=2048 -DEXTRA_SERIAL_CONFIG

slowtake_CLCFLAGS = $(CLCFLAGS) \
	-DPROGRAM_NAME='"SLOWTAKE"' \
	-DBUFFER_SIZE=2048  --asm-define BUFFER_SIZE=2048 \
	-DBAND_HEIGHT=20    --asm-define BAND_HEIGHT=20 \
	-DFILE_WIDTH=256    --asm-define FILE_WIDTH=256 \
	-DEXTRA_SERIAL_CONFIG --asm-define EXTRA_SERIAL_CONFIG \
	--asm-define AUXHGR_NO_AUTO_REMOVE \
	-C ../../config/apple2enh-slowtake.cfg

# 4 full tracks cache!
qktkconv_CLCFLAGS = \
	-DCACHE_SIZE=20480  --asm-define CACHE_SIZE=20480 \
	-DRAW_X_OFFSET=1    --asm-define RAW_X_OFFSET=1 \
	-DRAW_Y_OFFSET=1    --asm-define RAW_Y_OFFSET=1 \
	-DRAW_WIDTH=768     --asm-define RAW_WIDTH=768 \
	-DBAND_HEIGHT=20    --asm-define BAND_HEIGHT=20 \
	-DFILE_WIDTH=256    --asm-define FILE_WIDTH=256 \
	-DQKTKCONV=1 \
	$(CLCFLAGS) -C ../../config/apple2enh-alignbss.cfg \
	--start-addr 0x1000  #Two file buffers

qktkconv_GCCCFLAGS = \
	-DCACHE_SIZE=20480 \
	-DRAW_X_OFFSET=1 \
	-DRAW_Y_OFFSET=1 \
	-DRAW_WIDTH=768 \
	-DBAND_HEIGHT=20 \
	-DFILE_WIDTH=256 \
	-DQKTKCONV=1 \

# 3.5 tracks cache
radcconv_CLCFLAGS = \
	-DCACHE_SIZE=13312  --asm-define CACHE_SIZE=13312 \
	-DINITIAL_CACHE_READ=2048 \
	-DRAW_X_OFFSET=0    --asm-define RAW_X_OFFSET=0 \
	-DRAW_Y_OFFSET=0    --asm-define RAW_Y_OFFSET=0 \
	-DRAW_WIDTH=384     --asm-define RAW_WIDTH=384 \
	-DBAND_HEIGHT=20    --asm-define BAND_HEIGHT=20 \
	-DDECODE_WIDTH=320  --asm-define DECODE_WIDTH=320 \
	-DDECODE_HEIGHT=240 --asm-define DECODE_HEIGHT=240 \
	-DFILE_WIDTH=256    --asm-define FILE_WIDTH=256 \
	-DRADCCONV=1 \
	$(CLCFLAGS) -C ../../config/apple2enh-alignbssdata.cfg \
	--start-addr 0x1000  #Two file buffers

radcconv_GCCCFLAGS = \
	-DCACHE_SIZE=14336 \
	-DRAW_X_OFFSET=0 \
	-DRAW_Y_OFFSET=0 \
	-DRAW_WIDTH=384 \
	-DBAND_HEIGHT=20 \
	-DDECODE_WIDTH=320 \
	-DFILE_WIDTH=256 \
	-DRADCCONV=1 \

jpegconv_CLCFLAGS = \
	-DCACHE_SIZE=4096   --asm-define CACHE_SIZE=4096 \
	-DRAW_X_OFFSET=0    --asm-define RAW_X_OFFSET=0 \
	-DRAW_Y_OFFSET=0    --asm-define RAW_Y_OFFSET=0 \
	-DRAW_WIDTH=512     --asm-define RAW_WIDTH=512 \
	-DBAND_HEIGHT=40    --asm-define BAND_HEIGHT=40 \
	-DINPUT_WIDTH=640   --asm-define INPUT_WIDTH=640 \
	-DINPUT_HEIGHT=480  --asm-define INPUT_HEIGHT=480 \
	-DDECODE_WIDTH=320  --asm-define DECODE_WIDTH=320 \
	-DDECODE_HEIGHT=240 --asm-define DECODE_HEIGHT=240 \
	-DFILE_WIDTH=256    --asm-define FILE_WIDTH=256 \
	-DJPEGCONV=1        --asm-define JPEGCONV=1 \
	$(CLCFLAGS) -C ../../config/apple2enh-alignbssdata.cfg \
	--start-addr 0x1000  #Two file buffers

jpegconv_GCCCFLAGS = \
	-DCACHE_SIZE=15360 \
	-DRAW_X_OFFSET=0 \
	-DRAW_Y_OFFSET=0 \
	-DRAW_WIDTH=512 \
	-DBAND_HEIGHT=40 \
	-DINPUT_WIDTH=640 \
	-DINPUT_HEIGHT=480 \
	-DDECODE_WIDTH=320 \
	-DDECODE_HEIGHT=240 \
	-DFILE_WIDTH=256 \
	-DJPEGCONV=1 \

GCCCFLAGS = -g -Wall -Wno-pointer-sign -Wno-char-subscripts \
	-Wno-unused-but-set-variable -Wno-unused-variable -Wno-unknown-pragmas \
	-fno-omit-frame-pointer \
	$(shell sdl-config --cflags)

GCCLDFLAGS = $(shell sdl-config --libs)

.PHONY: all clean

all: quicktake.po microserial.x86_64

../image-viewer/imgview.bin:
	make -C ../image-viewer/ clean all

ABOUT.ZX: ui/about.in
	sed "s/__VERSION__/v$(VERSION)/" $< > about.tmp; \
	zx02 -f about.tmp ABOUT.ZX; \
	rm about.tmp

QT1X0.DRV: slowtake.bin
FUJI.DRV: slowtake.bin
DC50.DRV: slowtake.bin
SRRA.DRV: slowtake.bin

%.ZX: %.DRV
	zx02 -f $< $@

add_disk: ABOUT.ZX QT1X0.ZX FUJI.ZX DC50.ZX SRRA.ZX
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)SLOWTAKE.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -d $(IMG) $(PREFIX)BASIC.SYSTEM; \
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)ABOUT.ZX BIN 0x2000 < ABOUT.ZX; \
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)QT1X0.ZX BIN 0xC00 < QT1X0.ZX; \
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)FUJI.ZX BIN 0xC00 < FUJI.ZX; \
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)DC50.ZX BIN 0xC00 < DC50.ZX; \
	java -jar ../../bin/ac.jar -p $(IMG) $(PREFIX)SRRA.ZX BIN 0xC00 < SRRA.ZX; \
	for r in *.bin ../image-viewer/imgview.bin; do \
		java -jar ../../bin/ac.jar -as $(IMG) $(PREFIX)$$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

quicktake.po: splash.h slowtake.bin qktkconv.bin radcconv.bin jpegconv.bin loader.x86_64 qktkconv.x86_64 radcconv.x86_64 jpegconv.x86_64 ../image-viewer/imgview.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ $(FLOP_NAME)
	IMG=$@ PREFIX= make add_disk

hdv: 
	../../bin/cp2/cp2 mkdir $(IMG) $(FLOP_NAME)
	../../bin/cp2/cp2 copy --recurse quicktake.po $(IMG):$(FLOP_NAME)

splash.h: ui/splash.png
	../surl-server/hgr-convert $<
	../../bin/make-splash.sh ui/splash.png
	rm -f ui/splash.png.hgr*

slowtake.bin: $(slowtake_SOURCES) $(slowtake_CC65SOURCES) 
	cl65 --debug-info -Wl --dbgfile,"slowtake.dbg" $(CFLAGS) $(slowtake_CLCFLAGS) -vm -m slowtake.map -o $@ $^ $(TARGET)-iobuf-0800.o

microserial.x86_64: $(microserial_SOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(microserial_GCCCFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

qktkconv.bin: $(qktkconv_CC65SOURCES) $(qktkconv_SOURCES)
	cl65 --debug-info -Wl --dbgfile,"qktkconv.dbg" $(CFLAGS) $(qktkconv_CLCFLAGS) -vm -m qktkconv.map -o $@ $^ $(TARGET)-iobuf-0800.o

qktkconv.x86_64: $(qktkconv_SOURCES) $(qktkconv_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $(qktkconv_GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

radcconv.bin: $(radcconv_CC65SOURCES) $(radcconv_SOURCES)
	cl65 --debug-info -Wl --dbgfile,"radcconv.dbg" $(CFLAGS) $(radcconv_CLCFLAGS) -vm -m radcconv.map -o $@ $^ $(TARGET)-iobuf-0800.o

radcconv.x86_64: $(radcconv_SOURCES) $(radcconv_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $(radcconv_GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

jpegconv.bin: $(jpegconv_CC65SOURCES) $(jpegconv_SOURCES)
	cl65 --debug-info -Wl --dbgfile,"jpegconv.dbg" $(CFLAGS) $(jpegconv_CLCFLAGS) -vm -m jpegconv.map -o $@ $^ $(TARGET)-iobuf-0800.o

jpegconv.x86_64: $(jpegconv_SOURCES) $(jpegconv_GCCSOURCES) $(GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $(jpegconv_GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

loader.x86_64: tools/loader.c
	gcc $(microserial_GCCCFLAGS) -g -O0 -o $@ -I/usr/include/SDL -I../lib $< ../lib/extended_conio.c ../lib/c/clrzone.c -lSDL_image -lSDL

clean:
	rm -f *.bin *.x86_64 *.o *.map splash.h splash.hgr *.dbg *.lbl splash.h *.hgr *.po *DRV *ZX

upload: clean all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/ && \
	cp *.bin *ZX /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/quicktake/ && \
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/

dist: clean quicktake.po
	mkdir -p ../../dist/
	cp quicktake.po ../../dist/
