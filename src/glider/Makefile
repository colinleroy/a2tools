ifdef IIGS
serial_hw_SOURCES := ../lib/serial/z8530/common.s ../lib/serial/z8530/no_irq.s
iigs_suffix := -iigs
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
CC65_TARGET = apple2enh
upload_subdir := /iigs
else
ifdef OLDII
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
iigs_FLAGS :=
suffix :=
upload_subdir :=
CC65_TARGET := apple2
STREAMER_VER := 6502
else
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
iigs_suffix :=
iigs_CFLAGS :=
CC65_TARGET = apple2enh
trgt = apple2c0
STREAMER_VER := 65c02
endif
endif

USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD

GCCCFLAGS = -g -O0 -DCONF_FILE_PATH=\"/etc/a2tools/tty.conf\" \
	-I ../lib -I ../stp -Wall -Wno-format \
	-Wno-unused-function -Wno-char-subscripts -Wno-unknown-pragmas \
	-Werror

.PHONY: all clean

glider_SOURCES := \
	main.s \
	mouse.s \
	draw_text.s \
	draw_sprite.s \
	logic.s \
	levels.s \
	level0.s \
	level1.s \
	level2.s \
	buffers.s \
	load_bg.s \
	dashboard.s \
	../lib/fastirq.s \
	../lib/extrazp.s \
	../lib/asm/hgr.s \
	../lib/hgr_addrs.c \
	../lib/asm/bcd.s \
	font.gen.s \
	balloon.gen.s \
	clock.gen.s \
	plane.gen.s \
	rubber_band.gen.s \
	rubber_band_reverted.gen.s \
	rubber_box.gen.s \
	socket.gen.s \
	switch.gen.s \


glider_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl -C ../../config/apple2enh-rtonce.cfg \
	-vm -m glider.map --start-addr 0x6000

all: \
	glider.bin \
	levela.hgr \
	levelb.hgr \
	levelc.hgr

levela.hgr: levela.png
	../surl-server/hgr-convert levela.png && mv levela.png.hgr levela.hgr
levelb.hgr: levelb.png
	../surl-server/hgr-convert levelb.png && mv levelb.png.hgr levelb.hgr
levelc.hgr: levelc.png
	../surl-server/hgr-convert levelc.png && mv levelc.png.hgr levelc.hgr


gen_sprite: gen_sprite.c
	gcc $(shell pkg-config --cflags SDL_image) gen_sprite.c $(shell pkg-config --libs SDL_image) -o $@

gen_font: gen_font.c
	gcc $(shell pkg-config --cflags SDL_image) gen_font.c $(shell pkg-config --libs SDL_image) -o $@

balloon.gen.s: gen_sprite
	./gen_sprite balloon.png 255
clock.gen.s: gen_sprite
	./gen_sprite clock.png 255
rubber_band.gen.s: gen_sprite
	./gen_sprite rubber_band.png 255
rubber_band_reverted.gen.s: gen_sprite
	./gen_sprite rubber_band_reverted.png 255
rubber_box.gen.s: gen_sprite
	./gen_sprite rubber_box.png 255
socket.gen.s: gen_sprite
	./gen_sprite socket.png 255
switch.gen.s: gen_sprite
	./gen_sprite switch.png 255
plane.gen.s: gen_sprite
	./gen_sprite plane.png 280
font.gen.s: gen_font
	./gen_font font.png 280

clean:
	rm -f *.bin *.o \
	levela.hgr \
	levelb.hgr \
	levelc.hgr \
	gen_sprite \
	gen_font \
	*.gen.s \
	*.gen.inc \
	*.dbg *.lbl *.map \
	*.hgr-preview.*

glider.bin: $(glider_CL65SOURCES) $(glider_SOURCES)
	cl65 -g -Wl --dbgfile,glider.dbg -Ln glider.lbl $(CFLAGS) $(iigs_CFLAGS) $(glider_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/ && \
	cp *.bin *.hgr /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/
