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
	draw_sprite.s \
	logic.s \
	levels.s \
	level0.s \
	background_buffers.s \
	load_bg.s \
	../lib/fastirq.s \
	../lib/extrazp.s \
	../lib/asm/hgr.s \
	../lib/hgr_addrs.c \
	clock.s \
	plane.s \


glider_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl -C ../../config/apple2enh-rtonce.cfg \
	-vm -m glider.map --start-addr 0x4000

all: \
	glider.bin \
	levela.hgr \

levela.hgr: levela.png
	../surl-server/hgr-convert levela.png && mv levela.png.hgr levela.hgr

gen_sprite: gen_sprite.c
	gcc $(shell pkg-config --cflags SDL_image) gen_sprite.c $(shell pkg-config --libs SDL_image) -o $@

clock.s: gen_sprite
	./gen_sprite clock.png 255
plane.s: gen_sprite
	./gen_sprite plane.png 280

clean:
	rm -f *.bin *.o *.map \
	levela.hgr \
	gen_sprite \
	clock.inc \
	clock.s \
	plane.inc \
	plane.s

glider.bin: $(glider_CL65SOURCES) $(glider_SOURCES)
	cl65 -g -Wl --dbgfile,glider.dbg -Ln glider.lbl $(CFLAGS) $(iigs_CFLAGS) $(glider_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/ && \
	cp *.bin *.hgr /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/
