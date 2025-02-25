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

levels_backgrounds = \
	levela.hgr \
	levelb.hgr \
	levelc.hgr \

sprites = \
	balloon.gen.s \
	clock.gen.s \
	knife.gen.s \
	mini_score.gen.s \
	mini_plane.gen.s \
	rubber_band.gen.s \
	rubber_band_reverted.gen.s \
	rubber_box.gen.s \
	socket.gen.s \
	switch.gen.s \

sounds = \
	bubble.snd.s \
	click.snd.s \
	croutch.snd.s \
	crash.snd.s \

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
	plane.gen.s \
	play_sound.gen.s \
	$(sounds) \
	$(sprites)

all: \
	glider.bin \
	$(levels_backgrounds)

glider_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl -C ../../config/apple2enh-rtonce.cfg \
	-vm -m glider.map --start-addr 0x6000

gen_sprite: gen_sprite.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_font: gen_font.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_sound_generator: gen_sound_generator.c sound.h
	gcc -g -O0 $< -o $@

gen_sound_clip: gen_sound_clip.c sound.h
	gcc -g -O0 $< -o $@

$(levels_backgrounds): %.hgr: assets/levels/%.png

%.hgr: assets/levels/%.png
	../surl-server/hgr-convert $^ && mv $^.hgr $@

$(sprites): %.gen.s: assets/sprites/%.png
%.gen.s: assets/sprites/%.png gen_sprite
	./gen_sprite $< 255

plane.gen.s: assets/sprites/plane.png gen_sprite
	./gen_sprite $< 280
font.gen.s: assets/fonts/font.png gen_font
	./gen_font $< 280

play_sound.gen.s: gen_sound_generator
	./gen_sound_generator > $@

$(sounds): %.snd.s: assets/sounds/%.wav
%.snd.s: assets/sounds/%.wav gen_sound_clip
	rm -f $<.raw
	sox $< -b 8 -e unsigned-integer -t raw $<.raw channels 1 rate -v 8000
	./gen_sound_clip $<.raw > $@
	rm $<.raw

clean:
	rm -f *.bin *.o \
	$(levels_backgrounds) \
	gen_sprite \
	gen_font \
	gen_sound_clip \
	gen_sound_generator \
	*.gen.s \
	*.gen.inc \
	*.snd.s \
	assets/*/*.hgr-preview.* \
	*.dbg *.lbl *.map \

glider.bin: $(glider_CL65SOURCES) $(glider_SOURCES)
	cl65 -g -Wl --dbgfile,glider.dbg -Ln glider.lbl $(CFLAGS) $(iigs_CFLAGS) $(glider_CLCFLAGS) -o $@ $^  $(CC65_TARGET)-iobuf-0800.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/ && \
	cp *.bin *.hgr /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/
