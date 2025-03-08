CC65_TARGET := apple2
STREAMER_VER := 6502
SAMPLING_HZ := 8000

ifndef GAME_LANG
GAME_LANG := en
endif

ifndef SRC_DIR
SRC_DIR := .
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
levels_code        = $(levels:%=$(SRC_DIR)/levels_data/%.s)

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
	$(SRC_DIR)/code/main.s \
	$(SRC_DIR)/code/keyboard.s \
	$(SRC_DIR)/code/mouse.s \
	$(SRC_DIR)/code/draw_text.s \
	$(SRC_DIR)/code/draw_sprite.s \
	$(SRC_DIR)/code/animate.s \
	$(SRC_DIR)/code/game_loop.s \
	$(SRC_DIR)/code/logic.s \
	$(SRC_DIR)/code/levels.s \
	$(SRC_DIR)/code/buffers.s \
	$(SRC_DIR)/code/load_data.s \
	$(SRC_DIR)/code/dashboard.s \
	$(SRC_DIR)/code/strings-$(GAME_LANG).s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/asm/bcd.s \
	$(SRC_DIR)/../lib/asm/hgr_tables.s \
	$(SRC_DIR)/../lib/fastirq.s \
	$(SRC_DIR)/../lib/platform_sleep.s \
	font.gen.s \
	plane.pixel_perfect.gen.s \
	play_sound.gen.s \
	splash.bg.gen.s \
	$(sounds) \
	$(sprites) \
	$(levels_backgrounds) \
	$(levels_code)

#Start at 0xC00 to leave 1kB for file IO buffer

glider_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl \
	-vm -m glider.map \
	--asm-include-dir $(shell pwd) \
	--asm-include-dir $(SRC_DIR) \
	#--asm-define UNKILLABLE

# Targets

all dist: \
	glider-$(GAME_LANG).po

gen_sprite: $(SRC_DIR)/generators/gen_sprite.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_font: $(SRC_DIR)/generators/gen_font.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_sound_player: $(SRC_DIR)/../sound_player_generator/gen_sound_player.c $(SRC_DIR)/../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -g -O0 $< -o $@

gen_sound_clip: $(SRC_DIR)/../sound_player_generator/gen_sound_clip.c $(SRC_DIR)/../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -g -O0 $< -o $@

$(levels_backgrounds): level%.bg.gen.s: level%.hgr

splash.bg.gen.s: splash.hgr
	$(SRC_DIR)/../../bin/make-ca65-hgr.sh $< SEG > $@

level%.bg.gen.s: level%.hgr
	$(SRC_DIR)/../../bin/make-ca65-hgr.sh $< SEG > $@

splash.hgr: $(SRC_DIR)/assets/intl/$(GAME_LANG)/splash.xcf
	convert -flatten $< $<.png
	$(SRC_DIR)/../surl-server/hgr-convert $<.png && mv $<.png.hgr $@
	rm -f $<.png $<.png.hgr-preview.png

%.hgr: $(SRC_DIR)/assets/levels/%.xcf
	convert -flatten $< $<.png
	$(SRC_DIR)/../surl-server/hgr-convert $<.png && mv $<.png.hgr $@
	rm -f $<.png $<.png.hgr-preview.png

$(sprites_pixel_perfect): %.pixel_perfect.gen.s: $(SRC_DIR)/assets/sprites/%.png
%.pixel_perfect.gen.s: $(SRC_DIR)/assets/sprites/%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect

$(sprites_unperfect): %.unperfect.gen.s: $(SRC_DIR)/assets/sprites/%.png
%.unperfect.gen.s: $(SRC_DIR)/assets/sprites/%.png gen_sprite
	./gen_sprite $< 255 unperfect

plane.pixel_perfect.gen.s: $(SRC_DIR)/assets/sprites/plane.png gen_sprite
	./gen_sprite $< 280 pixel_perfect

font.gen.s: $(SRC_DIR)/assets/intl/$(GAME_LANG)/font.png gen_font
	./gen_font $< 280

play_sound.gen.s: gen_sound_player
	./gen_sound_player $(SAMPLING_HZ) > $@

$(sounds): %.snd.s: $(SRC_DIR)/assets/sounds/%.wav
%.snd.s: $(SRC_DIR)/assets/sounds/%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw > $@
	rm $<_raw

glider.ld.cfg: $(SRC_DIR)/glider.ld.cfg.tmpl
	$(SRC_DIR)/generators/gen_linker_config.sh $< $(levels) > $@

clean:
	rm -f *.bin *.o *.po \
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

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
glider-$(GAME_LANG).po: glider.bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ GLIDER.$(GAME_LANG)
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ GLIDER.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ GLIDER < $<; \
	for r in splash.bin lowcode.bin level_*.bin; do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ $$(basename $$r | sed "s/\.bin$///") BIN < $$r; \
	done;

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider-$(GAME_LANG)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider-$(GAME_LANG)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/glider-$(GAME_LANG)/

gimp_setup:
	./gimp_setup.sh
