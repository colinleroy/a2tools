ifdef IIGS
serial_hw_SOURCES := ../lib/serial/z8530/common.s ../lib/serial/z8530/no_irq.s
iigs_suffix := -iigs
iigs_CFLAGS := -DIIGS --asm-define IIGS
trgt = apple2gs
CC65_TARGET = apple2enh
upload_subdir := /iigs
STREAMER_VER := 65c02
SAMPLING_HZ := 8000
else
ifdef OLDII
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
iigs_FLAGS :=
suffix :=
upload_subdir :=
CC65_TARGET := apple2
STREAMER_VER := 6502
SAMPLING_HZ := 4000
else
serial_hw_SOURCES := ../lib/serial/acia/common.s ../lib/serial/acia/no_irq.s
iigs_suffix :=
iigs_CFLAGS :=
CC65_TARGET = apple2enh
trgt = apple2c0
STREAMER_VER := 65c02
SAMPLING_HZ := 8000
endif
endif

ifndef GAME_LANG
GAME_LANG := en
GAME_CHARSET := latin
endif


BINARY_NAME = glider.bin

.PHONY: all clean

# Generated things

levels = \
	level_a \
	level_b \
	level_c \
	level_d \
	level_e \
	level_f \

levels_backgrounds = $(levels:=.bg.gen.s)
levels_code        = $(levels:%=levels_data/%.s)

sprites_pixel_perfect = \
	knife.pixel_perfect.gen.s \
	toast.pixel_perfect.gen.s \
	plane_destroyed.pixel_perfect.gen.s \
	rubber_band.pixel_perfect.gen.s \

sprites_unperfect = \
	balloon.unperfect.gen.s \
	battery.unperfect.gen.s \
	battery_reverted.unperfect.gen.s \
	chainsaw.unperfect.gen.s \
	clock.unperfect.gen.s \
	mini_score.unperfect.gen.s \
	mini_plane.unperfect.gen.s \
	rubber_band_reverted.unperfect.gen.s \
	rubber_box.unperfect.gen.s \
	sheet.unperfect.gen.s \
	socket.unperfect.gen.s \
	switch.unperfect.gen.s \

sprites = \
	$(sprites_pixel_perfect) \
	$(sprites_unperfect)

sounds = \
	bubble.snd.s \
	chainsaw.snd.s \
	click.snd.s \
	crash.snd.s \
	croutch.snd.s \
	ding.snd.s \

# Our code

glider_SOURCES := \
	code/main.s \
	code/keyboard.s \
	code/mouse.s \
	code/draw_text.s \
	code/draw_sprite.s \
	code/animate.s \
	code/game_loop.s \
	code/logic.s \
	code/levels.s \
	code/buffers.s \
	code/load_data.s \
	code/dashboard.s \
	code/strings-$(GAME_LANG).s \
	../lib/extrazp.s \
	../lib/asm/hgr.s \
	../lib/asm/bcd.s \
	../lib/asm/hgr_tables.s \
	../lib/fastirq.s \
	../lib/platform_sleep.s \
	font.gen.s \
	plane.pixel_perfect.gen.s \
	play_sound.gen.s \
	splash.bg.gen.s \
	$(sounds) \
	$(sprites) \
	$(levels_backgrounds) \
	$(levels_code) \

all: \
	glider.bin

#Start at 0xC00 to leave 1kB for file IO buffer

glider_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl \
	-vm -m glider.map \
	--asm-include-dir $(shell pwd) \
	#--asm-define UNKILLABLE

gen_sprite: generators/gen_sprite.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_font: generators/gen_font.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_sound_player: ../sound_player_generator/gen_sound_player.c ../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -g -O0 $< -o $@

gen_sound_clip: ../sound_player_generator/gen_sound_clip.c ../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -g -O0 $< -o $@

$(levels_backgrounds): level%.bg.gen.s: level%.hgr

splash.bg.gen.s: splash.hgr
	../../bin/make-ca65-hgr.sh $< SEG > $@

level%.bg.gen.s: level%.hgr
	../../bin/make-ca65-hgr.sh $< SEG > $@

splash.hgr: assets/levels/splash-$(GAME_LANG).xcf
	cp assets/levels/splash-$(GAME_LANG).xcf splash-$(GAME_LANG).xcf
	convert -flatten $< $<.png
	../surl-server/hgr-convert $<.png && mv $<.png.hgr $@
	rm -f $<.png $<.png.hgr-preview.png splash-$(GAME_LANG).xcf

%.hgr: assets/levels/%.xcf
	convert -flatten $< $<.png
	../surl-server/hgr-convert $<.png && mv $<.png.hgr $@
	rm -f $<.png $<.png.hgr-preview.png

$(sprites_pixel_perfect): %.pixel_perfect.gen.s: assets/sprites/%.png
%.pixel_perfect.gen.s: assets/sprites/%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect

$(sprites_unperfect): %.unperfect.gen.s: assets/sprites/%.png
%.unperfect.gen.s: assets/sprites/%.png gen_sprite
	./gen_sprite $< 255 unperfect

plane.pixel_perfect.gen.s: assets/sprites/plane.png gen_sprite
	./gen_sprite $< 280 pixel_perfect

font.gen.s: assets/fonts/font-$(GAME_CHARSET).png gen_font
	cp -f assets/fonts/font-$(GAME_CHARSET).png font.png
	./gen_font font.png 280
	rm font.png

play_sound.gen.s: gen_sound_player
	./gen_sound_player $(SAMPLING_HZ) > $@

$(sounds): %.snd.s: assets/sounds/%.wav
%.snd.s: assets/sounds/%.wav gen_sound_clip
	rm -f $<.raw
	sox $< -b 8 -e unsigned-integer -t raw $<.raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<.raw > $@
	rm $<.raw

glider.ld.cfg: glider.ld.cfg.tmpl
	./generators/gen_linker_config.sh $< $(levels) > $@

clean:
	rm -f *.bin *.o \
	$(levels_backgrounds) \
	gen_sprite \
	gen_font \
	gen_sound_clip \
	gen_sound_player \
	*.gen.s \
	*.gen.inc \
	*.snd.s \
	splash.hgr \
	level*.hgr \
	assets/*/*.hgr-preview.* \
	*.dbg *.lbl *.map \
	glider.ld.cfg

glider.bin: glider.ld.cfg $(glider_SOURCES)
	cl65 -g -Wl --dbgfile,glider.dbg -Ln glider.lbl $(CFLAGS) $(iigs_CFLAGS) $(glider_CLCFLAGS) \
		-C glider.ld.cfg -o $@ $(glider_SOURCES) \
		$(CC65_TARGET)-iobuf-0800.o $(CC65_TARGET)-integer-basic-compat.o

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider/

gimp_setup:
	./gimp_setup.sh
