CC65_TARGET := apple2
STREAMER_VER := 6502
SAMPLING_HZ := 8000
# We have a 0x2100 buffer for uncompression,
# and want to make sure the end is a bit further
MAX_COMPRESSED_DATA_SIZE := 0x2040

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
	level_g \
	level_h \
	level_i \
	level_j \
	level_k \
	level_l \
	level_m \
	level_n \

lz4_files = \
	$(levels) \
	splash \
	lowcode \

levels_backgrounds = $(levels:=.bg.gen.s)
levels_code        = $(levels:%=$(SRC_DIR)/levels_data/%.s)
assets_bin         = $(lz4_files:=.bin)
assets_lz4         = $(lz4_files:=.lz4)

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
	$(SRC_DIR)/code/hz.s \
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
	$(SRC_DIR)/code/scores.s \
	$(SRC_DIR)/code/strings-$(GAME_LANG).s \
	$(SRC_DIR)/../lib/fastirq.s \
	$(SRC_DIR)/../lib/asm/hgr_tables.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/asm/hgr_mono40.s \
	$(SRC_DIR)/../lib/asm/hgr_setup_mono40.s \
	$(SRC_DIR)/../lib/asm/auxhgr_file.s \
	$(SRC_DIR)/../lib/asm/bcd.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/cutoa.s \
	$(SRC_DIR)/../lib/cntoabuf.s \
	$(SRC_DIR)/../lib/platform_sleep.s \
	font.gen.s \
	plane.pixel_perfect.gen.s \
	play_sound.gen.s \
	splash.bg.gen.s \
	$(sounds) \
	$(sprites) \
	$(levels_backgrounds) \
	$(levels_code)

ifdef UNKILLABLE
UNKILLABLE_CFLAG = --asm-define UNKILLABLE
endif

glider_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl \
	-vm -m glider.map \
	--asm-include-dir $(shell pwd) \
	--asm-include-dir $(SRC_DIR) \
	--asm-define MAX_COMPRESSED_DATA_SIZE=$(MAX_COMPRESSED_DATA_SIZE) \
	$(UNKILLABLE_CFLAG)

# Targets

all dist: \
	glider-$(GAME_LANG).po

gen_sprite: $(SRC_DIR)/../gen_sprite/gen_sprite.c
	gcc -DENABLE_QUICK_DRAW=1 -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_font: $(SRC_DIR)/../gen_font/gen_font.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_sound_player: $(SRC_DIR)/../sound_player_generator/gen_sound_player.c $(SRC_DIR)/../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -g -O0 $< -o $@

gen_sound_clip: $(SRC_DIR)/../sound_player_generator/gen_sound_clip.c $(SRC_DIR)/../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -g -O0 $< -o $@

lz4_compressor: $(SRC_DIR)/../lz4_compressor/lz4_compressor.c
	gcc -g -O0 -DMAX_COMPRESSED_DATA_SIZE=$(MAX_COMPRESSED_DATA_SIZE) $^ -llz4 -o $@

$(assets_bin): $(BINARY_NAME)

%.lz4: %.bin lz4_compressor
	./lz4_compressor $< > $@

$(levels_backgrounds): level%.bg.gen.s: level%.hgr

splash.bg.gen.s: splash.hgr
	$(SRC_DIR)/../../bin/make-ca65-hgr.sh $< SEG > $@

level%.bg.gen.s: level%.hgr
	$(SRC_DIR)/../../bin/make-ca65-hgr.sh $< SEG > $@

$(SRC_DIR)/../surl-server/hgr-convert:
	$(MAKE) -C $(SRC_DIR)/../surl-server/ -f Makefile

splash.hgr: $(SRC_DIR)/assets/intl/$(GAME_LANG)/splash.xcf $(SRC_DIR)/../surl-server/hgr-convert
	convert -flatten $< $<.png
	$(SRC_DIR)/../surl-server/hgr-convert $<.png && mv $<.png.hgr $@
	rm -f $<.png $<.png.hgr-preview.png

%.hgr: $(SRC_DIR)/assets/levels/%.xcf $(SRC_DIR)/../surl-server/hgr-convert
	convert -flatten $< $<.png
	$(SRC_DIR)/../surl-server/hgr-convert $<.png && mv $<.png.hgr $@
	rm -f $<.png $<.png.hgr-preview.png

$(sprites_pixel_perfect): %.pixel_perfect.gen.s: $(SRC_DIR)/assets/sprites/%.png
%.pixel_perfect.gen.s: $(SRC_DIR)/assets/sprites/%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect

$(sprites_unperfect): %.unperfect.gen.s: $(SRC_DIR)/assets/sprites/%.png
%.unperfect.gen.s: $(SRC_DIR)/assets/sprites/%.png gen_sprite
	./gen_sprite $< 255 unperfect DATA align

plane.pixel_perfect.gen.s: $(SRC_DIR)/assets/sprites/plane.png gen_sprite
	./gen_sprite $< 280 pixel_perfect DATA align

font.gen.s: $(SRC_DIR)/assets/fonts/font.png gen_font
	./gen_font $< 280

play_sound.gen.s: gen_sound_player
	./gen_sound_player $(SAMPLING_HZ) > $@

$(sounds): %.snd.s: $(SRC_DIR)/assets/sounds/%.wav
%.snd.s: $(SRC_DIR)/assets/sounds/%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -R -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
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
	lz4_compressor \
	*.gen.s \
	*.gen.inc \
	*.snd.s \
	splash.hgr \
	level*.hgr \
	assets/*/*.hgr-preview.* \
	*.dbg *.lbl *.map *.lz4 \
	glider.ld.cfg \
	check_imports.sh

check_imports.sh: $(glider_SOURCES) $(SRC_DIR)/check_imports.sh
	cp -f $(SRC_DIR)/check_imports.sh .
	./check_imports.sh $(SRC_DIR)/code
	./check_imports.sh $(SRC_DIR)/levels_data

$(BINARY_NAME): glider.ld.cfg $(glider_SOURCES) check_imports.sh
	cl65 -g -Wl --dbgfile,glider.dbg -Ln glider.lbl $(CFLAGS) $(glider_CLCFLAGS) \
		-C glider.ld.cfg -o $@ $(glider_SOURCES) \
		$(CC65_TARGET)-iobuf-0800.o $(CC65_TARGET)-integer-basic-compat.o

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
glider-$(GAME_LANG).po: $(BINARY_NAME) $(assets_lz4) $(SRC_DIR)/assets/SCORES.bin
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ GLIDER.$(GAME_LANG)
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ GLIDER.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ GLIDER < $<; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ SCORES BIN < $(SRC_DIR)/assets/SCORES.bin; \
	for r in $(assets_lz4); do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ $$(basename $$r | sed "s/\.lz4$///") BIN < $$r; \
	done;

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/glider/$(GAME_LANG)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/glider/$(GAME_LANG)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/glider/$(GAME_LANG)/
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/ && \
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2/disks/

gimp_setup:
	./gimp_setup.sh

dist: glider-$(GAME_LANG).po
	mkdir -p $(SRC_DIR)/../../dist/
	cp $< $(SRC_DIR)/../../dist/
