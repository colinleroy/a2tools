CC65_TARGET := apple2
STREAMER_VER := 6502
SAMPLING_HZ := 8000

ifndef SRC_DIR
SRC_DIR := .
endif

BINARY_NAME = skel.bin

.PHONY: all clean

# Generated things

sprites_pixel_perfect = \
	puck0.pixel_perfect.gen.s

sprites = \
	$(sprites_pixel_perfect) \

sounds = \
	puck_hit.snd.s \

zx02_files = \
        $(assets_files) \
        low_code \
				lc_code \

assets_zx02         = $(zx02_files:=.zx02)

# Our code
# Order is important so that things are linked in the expected order
# in the various segments, and things needing to be aligned are aligned
skel_SOURCES := \
	code/buffers.s \
	play_sound.gen.s \
	code/init.s \
	code/main.s \
	code/hz.s \
	code/mouse.s \
	code/draw_sprite.s \
	code/game_loop.s \
	code/sprites.s \
	code/mini_conio.s \
	code/load_data.s \
	../lib/asm/auxhgr_file.s \
	../lib/asm/hgr.s \
	../lib/asm/hgr_mono40.s \
	../lib/asm/hgr_setup_mono40.s \
	../lib/asm/hgr_tables.s \
	../lib/cntoabuf.s \
	../lib/extrazp.s \
	splash.bg.gen.s \
	$(sprites)

skel_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl \
	-vm -m skel.map \
	--asm-include-dir $(shell pwd) \
	--asm-define DISABLE_DHGR \

# Targets

all dist: \
	skel.po

gen_sprite: ../gen_sprite/gen_sprite.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_font: ../gen_font/gen_font.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_sound_player: ../sound_player_generator/gen_sound_player.c ../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -DENABLE_SLOWER=1 -g -O0 $< -o $@

gen_sound_clip: ../sound_player_generator/gen_sound_clip.c ../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -DENABLE_SLOWER=1 -g -O0 $< -o $@

$(assets_bin): $(BINARY_NAME)

%.zx02: %.bin
	zx02 -f $< $@

$(assets_backgrounds): %.bg.gen.s: %.hgr

%.bg.gen.s: %.hgr
	../../bin/make-ca65-hgr.sh $< SEG > $@

../surl-server/hgr-convert:
	$(MAKE) -C ../surl-server/ -f Makefile

%.hgr: assets/%.xcf ../surl-server/hgr-convert
	convert -flatten $< $<.png
	#Use -short to have 8 less bytes at the end so zx02 can uncompress
	#in-place
	../surl-server/hgr-convert -short $<.png && mv $<.png.hgr $@
	rm -f $<.png $<.png.hgr-preview.png

$(sprites_pixel_perfect): %.pixel_perfect.gen.s: assets/%.png
puck%.pixel_perfect.gen.s: assets/puck%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SPLC

play_sound.gen.s: gen_sound_player
	./gen_sound_player $(SAMPLING_HZ) LOWCODE > $@

$(sounds): %.snd.s: assets/sounds/%.wav
puc%.snd.s: assets/sounds/puc%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -R -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw SPLC > $@
	rm $<_raw

font.gen.s: assets/fonts/font.png gen_font
	./gen_font $< 280 barcode

skel.ld.cfg: skel.ld.cfg.tmpl generators/gen_linker_config.sh
	generators/gen_linker_config.sh $< $(opponents) > $@

clean:
	rm -f *.bin */*.o *.o *.po \
	$(assets_backgrounds) \
	gen_sprite \
	gen_font \
	gen_sound_clip \
	gen_sound_player \
	gen_transform \
	*.gen.s \
	*.gen.inc \
	*.snd.s \
	transform.s \
	splash.hgr \
	table.hgr \
	level*.hgr \
	assets/*/*.hgr-preview.* \
	*.dbg *.lbl *.map *.zx02 \
	skel.ld.cfg

check_imports.sh: $(skel_SOURCES) check_imports.sh
	cp -f check_imports.sh .
	./check_imports.sh code
	./check_imports.sh levels_data

$(BINARY_NAME): skel.ld.cfg $(skel_SOURCES)
	cl65 -g -Wl --dbgfile,skel.dbg -Ln skel.lbl $(CFLAGS) $(skel_CLCFLAGS) \
		-C skel.ld.cfg -o $@ $(skel_SOURCES) \
		$(CC65_TARGET)-iobuf-0800.o $(CC65_TARGET)-integer-basic-compat.o

CLEANDISK = ../../disks/ProDOS_2_4_3.po
skel.po: $(BINARY_NAME) $(assets_zx02)
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ SKEL
	java -jar ../../bin/ac.jar -p $@ SKEL.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar ../../bin/ac.jar -as $@ SKEL < $<; \
	for r in $(assets_zx02); do \
		java -jar ../../bin/ac.jar -p $@ $$(basename $$r | sed "s/\.zx02$///") BIN < $$r; \
	done; \
# 
# upload: all
# 	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/ || \
# 	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/ && \
# 	cp *.bin *.zx02 /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/
# 	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/ || \
# 	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/ && \
# 	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/

dist: skel.po
	mkdir -p ../../dist/
	cp $< ../../dist/
