CC65_TARGET := apple2
STREAMER_VER := 6502
SAMPLING_HZ := 8000
# We have a 0x2100 buffer for uncompression,
# and want to make sure the end is a bit further
MAX_COMPRESSED_DATA_SIZE := 0x2040

ifndef SRC_DIR
SRC_DIR := .
endif

BINARY_NAME = shflpuck.bin

.PHONY: all clean

# Generated things

assets = \
	table

lz4_files = \
	$(assets) \
	lowcode

assets_backgrounds = $(assets:=.bg.gen.s)
assets_bin         = $(lz4_files:=.bin)
assets_lz4         = $(lz4_files:=.lz4)

sprites_pixel_perfect = \
	my_pusher0.pixel_perfect.gen.s \
	my_pusher1.pixel_perfect.gen.s \
	my_pusher2.pixel_perfect.gen.s \
	my_pusher3.pixel_perfect.gen.s \
	their_pusher4.pixel_perfect.gen.s \
	their_pusher5.pixel_perfect.gen.s \
	puck0.pixel_perfect.gen.s \
	puck1.pixel_perfect.gen.s \
	puck2.pixel_perfect.gen.s \
	puck3.pixel_perfect.gen.s \
	puck4.pixel_perfect.gen.s \
	puck5.pixel_perfect.gen.s \
	puck6.pixel_perfect.gen.s \

sprites = \
	$(sprites_pixel_perfect)

sounds = \
	puck_hit.snd.s \
	crash.snd.s \
	puck.snd.s \
# Our code

shflpuck_SOURCES := \
	play_sound.gen.s \
	$(SRC_DIR)/code/main.s \
	$(SRC_DIR)/code/mouse.s \
	$(SRC_DIR)/code/crash_lines.s \
	$(SRC_DIR)/code/draw_sprite.s \
	$(SRC_DIR)/code/draw_score.s \
	$(SRC_DIR)/code/game_loop.s \
	$(SRC_DIR)/code/buffers.s \
	$(SRC_DIR)/code/load_data.s \
	$(SRC_DIR)/code/sprites.s \
	$(SRC_DIR)/code/opponent.s \
	$(SRC_DIR)/../lib/asm/hgr_tables.s \
	$(SRC_DIR)/../lib/asm/hgr.s \
	$(SRC_DIR)/../lib/asm/bcd.s \
	$(SRC_DIR)/../lib/extrazp.s \
	$(SRC_DIR)/../lib/cutoa.s \
	$(SRC_DIR)/../lib/cntoabuf.s \
	$(SRC_DIR)/../lib/platform_sleep.s \
	$(sounds) \
	transform.s \
	$(sprites) \
	$(assets_backgrounds) \
	$(levels_code)

shflpuck_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl \
	-vm -m shflpuck.map \
	--asm-include-dir $(shell pwd) \
	--asm-include-dir $(SRC_DIR) \
	--asm-define MAX_COMPRESSED_DATA_SIZE=$(MAX_COMPRESSED_DATA_SIZE) \
	$(UNKILLABLE_CFLAG)

# Targets

all dist: \
	shufflepuck.po

gen_sprite: $(SRC_DIR)/../gen_sprite/gen_sprite.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_font: $(SRC_DIR)/generators/gen_font.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_sound_player: $(SRC_DIR)/../sound_player_generator/gen_sound_player.c $(SRC_DIR)/../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -DENABLE_SLOWER=1 -g -O0 $< -o $@

gen_sound_clip: $(SRC_DIR)/../sound_player_generator/gen_sound_clip.c $(SRC_DIR)/../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -DENABLE_SLOWER=1 -g -O0 $< -o $@

gen_transform: $(SRC_DIR)/generators/gen_transform.c
	gcc -g -O0 $< -o $@

transform.s: gen_transform
	./$< > $@

lz4_compressor: $(SRC_DIR)/../glider/generators/lz4_compressor.c
	gcc -g -O0 -DMAX_COMPRESSED_DATA_SIZE=$(MAX_COMPRESSED_DATA_SIZE) $^ -llz4 -o $@

$(assets_bin): $(BINARY_NAME)

%.lz4: %.bin lz4_compressor
	./lz4_compressor $< > $@

$(assets_backgrounds): %.bg.gen.s: %.hgr

%.bg.gen.s: %.hgr
	$(SRC_DIR)/../../bin/make-ca65-hgr.sh $< SEG > $@

$(SRC_DIR)/../surl-server/hgr-convert:
	$(MAKE) -C $(SRC_DIR)/../surl-server/ -f Makefile

%.hgr: $(SRC_DIR)/assets/%.xcf $(SRC_DIR)/../surl-server/hgr-convert
	convert -flatten $< $<.png
	$(SRC_DIR)/../surl-server/hgr-convert $<.png && mv $<.png.hgr $@
	rm -f $<.png $<.png.hgr-preview.png

$(sprites_pixel_perfect): %.pixel_perfect.gen.s: $(SRC_DIR)/assets/%.png
my%.pixel_perfect.gen.s: $(SRC_DIR)/assets/my%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect RODATA
their%.pixel_perfect.gen.s: $(SRC_DIR)/assets/their%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect LC
puck%.pixel_perfect.gen.s: $(SRC_DIR)/assets/puck%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect RODATA

play_sound.gen.s: gen_sound_player
	./gen_sound_player $(SAMPLING_HZ) LOWCODE > $@

$(sounds): %.snd.s: $(SRC_DIR)/assets/sounds/%.wav
crash.snd.s: $(SRC_DIR)/assets/sounds/crash.wav gen_sound_clip
	rm -f $<_raw
	sox $< -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw RODATA > $@
	rm $<_raw
puc%.snd.s: $(SRC_DIR)/assets/sounds/puc%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw LC > $@
	rm $<_raw

shflpuck.ld.cfg: $(SRC_DIR)/shflpuck.ld.cfg.tmpl
	$(SRC_DIR)/../glider/generators/gen_linker_config.sh $< $(assets) > $@

clean:
	rm -f *.bin *.o *.po \
	$(assets_backgrounds) \
	gen_sprite \
	gen_font \
	gen_sound_clip \
	gen_sound_player \
	gen_transform \
	lz4_compressor \
	*.gen.s \
	*.gen.inc \
	*.snd.s \
	transform.s \
	table.hgr \
	level*.hgr \
	assets/*/*.hgr-preview.* \
	*.dbg *.lbl *.map *.lz4 \
	shflpuck.ld.cfg \
	check_imports.sh

check_imports.sh: $(shflpuck_SOURCES) $(SRC_DIR)/check_imports.sh
	cp -f $(SRC_DIR)/check_imports.sh .
	./check_imports.sh $(SRC_DIR)/code
	./check_imports.sh $(SRC_DIR)/levels_data

$(BINARY_NAME): shflpuck.ld.cfg $(shflpuck_SOURCES)
	cl65 -g -Wl --dbgfile,shflpuck.dbg -Ln shflpuck.lbl $(CFLAGS) $(iigs_CFLAGS) $(shflpuck_CLCFLAGS) \
		-C shflpuck.ld.cfg -o $@ $(shflpuck_SOURCES) \
		$(CC65_TARGET)-iobuf-0800.o $(CC65_TARGET)-integer-basic-compat.o

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
shufflepuck.po: $(BINARY_NAME) $(assets_lz4)
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ SHUFFLEPUCK
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ SHFLPUCK.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ SHFLPUCK < $<; \
	for r in $(assets_lz4); do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ $$(basename $$r | sed "s/\.lz4$///") BIN < $$r; \
	done;

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/ && \
	cp *.bin *.lz4 /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/ && \
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/

gimp_setup:
	./gimp_setup.sh

dist: shufflepuck.po
	mkdir -p $(SRC_DIR)../../dist/
	cp $< $(SRC_DIR)../../dist/
