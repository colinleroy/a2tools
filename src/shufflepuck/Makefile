CC65_TARGET := apple2
STREAMER_VER := 6502
SAMPLING_HZ := 8000
# We have a 0x2100 buffer for uncompression,
# and want to make sure the end is a bit further
MAX_COMPRESSED_DATA_SIZE := 0x2040

ifndef SRC_DIR
SRC_DIR := .
endif

BINARY_NAME = shufflepuck.bin

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

# Our code

shufflepuck_SOURCES := \
	$(SRC_DIR)/code/main.s \
	$(SRC_DIR)/code/mouse.s \
	$(SRC_DIR)/code/draw_sprite.s \
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
	transform.s \
	$(sounds) \
	$(sprites) \
	$(assets_backgrounds) \
	$(levels_code)

shufflepuck_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl \
	-vm -m shufflepuck.map \
	--asm-include-dir $(shell pwd) \
	--asm-include-dir $(SRC_DIR) \
	--asm-define MAX_COMPRESSED_DATA_SIZE=$(MAX_COMPRESSED_DATA_SIZE) \
	$(UNKILLABLE_CFLAG)

# Targets

all dist: \
	shufflepuck.po

gen_sprite: $(SRC_DIR)/../glider/generators/gen_sprite.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_font: $(SRC_DIR)/generators/gen_font.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_sound_player: $(SRC_DIR)/../sound_player_generator/gen_sound_player.c $(SRC_DIR)/../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -g -O0 $< -o $@

gen_sound_clip: $(SRC_DIR)/../sound_player_generator/gen_sound_clip.c $(SRC_DIR)/../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -g -O0 $< -o $@

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
%.pixel_perfect.gen.s: $(SRC_DIR)/assets/%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect

play_sound.gen.s: gen_sound_player
	./gen_sound_player $(SAMPLING_HZ) > $@

$(sounds): %.snd.s: $(SRC_DIR)/assets/sounds/%.wav
%.snd.s: $(SRC_DIR)/assets/sounds/%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw > $@
	rm $<_raw

shufflepuck.ld.cfg: $(SRC_DIR)/shufflepuck.ld.cfg.tmpl
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
	shufflepuck.ld.cfg \
	check_imports.sh

check_imports.sh: $(shufflepuck_SOURCES) $(SRC_DIR)/check_imports.sh
	cp -f $(SRC_DIR)/check_imports.sh .
	./check_imports.sh $(SRC_DIR)/code
	./check_imports.sh $(SRC_DIR)/levels_data

$(BINARY_NAME): shufflepuck.ld.cfg $(shufflepuck_SOURCES)
	cl65 -g -Wl --dbgfile,shufflepuck.dbg -Ln shufflepuck.lbl $(CFLAGS) $(iigs_CFLAGS) $(shufflepuck_CLCFLAGS) \
		-C shufflepuck.ld.cfg -o $@ $(shufflepuck_SOURCES) \
		$(CC65_TARGET)-iobuf-0800.o $(CC65_TARGET)-integer-basic-compat.o

CLEANDISK = $(SRC_DIR)/../../disks/ProDOS_2_4_3.po
shufflepuck.po: $(BINARY_NAME) $(assets_lz4)
	cp $(CLEANDISK) $@; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -n $@ SHFLPUCK
	java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ SHFLPUCK.SYSTEM SYS < $(SRC_DIR)/../../bin/loader.system; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar $(SRC_DIR)/../../bin/ac.jar -as $@ SHFLPUCK < $<; \
	for r in $(assets_lz4); do \
		java -jar $(SRC_DIR)/../../bin/ac.jar -p $@ $$(basename $$r | sed "s/\.lz4$///") BIN < $$r; \
	done;

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/ && \
	cp *.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/ && \
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/

gimp_setup:
	./gimp_setup.sh

dist: shufflepuck.po
	mkdir -p $(SRC_DIR)../../dist/
	cp $< $(SRC_DIR)../../dist/
