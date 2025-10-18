CC65_TARGET := apple2
STREAMER_VER := 6502
SAMPLING_HZ := 8000
MAX_COMPRESSED_DATA_SIZE := 0x2D00

DEBUG_OPPONENT_CFLAG :=

ifndef SRC_DIR
SRC_DIR := .
endif

# A: Skip
# B: Visine
# C: Vinnie
# D: Lexan
# E: Eneg
# F: Nerual
# G: Bejin
# H: Biff
# I: DC3
# N: network (serial)

opponents = \
	a \
	b \
	c \
	d \
	e \
	f \
	g \
	h \
	i \
	n \

opponents_with_sprites = \
	a \
	b \
	c \
	d \
	e \
	f \
	g \
	h \
	i \

ifdef DEBUG_OPPONENT
	DEBUG_OPPONENT_NUM := $(shell printf "%d" "'$(DEBUG_OPPONENT)'")
	DEBUG_OPPONENT_NUM := $(shell echo $$(($(DEBUG_OPPONENT_NUM) - 97)))
	DEBUG_OPPONENT_CFLAG := --asm-define DEBUG_OPPONENT=$(DEBUG_OPPONENT_NUM)
endif

BINARY_NAME = shflpuck.bin

.PHONY: all clean

# Generated things

assets = \
	bar \
	table \
	hallfame \

assets_files = $(assets:%=%_img)

opponents_code    = $(opponents:%=opponents/opponent%.s)
opponents_files   = $(opponents:%=opponent_%)

zx02_files = \
	$(assets_files) \
	$(opponents_files) \
	low_code \
	lc_code \
	bar_snd \
	bar_code \
	sdata \

assets_backgrounds = $(assets:=.bg.gen.s)
assets_bin         = $(zx02_files:=.bin)
assets_zx02         = $(zx02_files:=.zx02)


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
	hand.pixel_perfect.gen.s \

sprites_unperfect = $(opponents_with_sprites:%=sprite_%.unperfect.gen.s)
names_unperfect   = $(opponents:%=name_%.unperfect.gen.s)

lose_unperfect = \
	lose_a.unperfect.gen.s \
	lose_b.unperfect.gen.s \
	lose_c.unperfect.gen.s \
	lose_d.unperfect.gen.s \
	lose_e.unperfect.gen.s \
	lose_f.unperfect.gen.s \
	lose_h.unperfect.gen.s \
	lose_n_4.unperfect.gen.s \
	lose_n_5.unperfect.gen.s \
	lose_n_6.unperfect.gen.s \
	losegame_c_1.unperfect.gen.s \
	losegame_c_2.unperfect.gen.s \
	losegame_c_3.unperfect.gen.s \
	losegame_c_4.unperfect.gen.s \

win_unperfect = \
	win_a.unperfect.gen.s \
	win_b.unperfect.gen.s \
	win_c.unperfect.gen.s \
	win_d_0.unperfect.gen.s \
	win_d_1.unperfect.gen.s \
	win_d_2.unperfect.gen.s \
	win_d_3.unperfect.gen.s \
	win_d_4.unperfect.gen.s \
	wingame_e_1.unperfect.gen.s \
	wingame_e_2.unperfect.gen.s \
	win_e_1.unperfect.gen.s \
	win_e_2.unperfect.gen.s \
	win_e_3.unperfect.gen.s \
	win_e_4.unperfect.gen.s \
	win_e_5.unperfect.gen.s \
	win_f.unperfect.gen.s \
	win_g.unperfect.gen.s \
	win_h.unperfect.gen.s \
	win_n_4.unperfect.gen.s \
	win_n_5.unperfect.gen.s \
	win_n_6.unperfect.gen.s \

serve_unperfect   = \
	serve_g_1.unperfect.gen.s \
	serve_g_2.unperfect.gen.s \
	serve_g_3.unperfect.gen.s \
	serve_g_4.unperfect.gen.s \

special_unperfect   = \
	sprite_n_1.unperfect.gen.s \
	sprite_n_2.unperfect.gen.s \
	sprite_n_3.unperfect.gen.s \
	sprite_n_4.unperfect.gen.s \
	sprite_n_5.unperfect.gen.s \
	sprite_n_6.unperfect.gen.s \
	sprite_n_7.unperfect.gen.s \
	normal_h.unperfect.gen.s \
	normal_i.unperfect.gen.s \
	moving_i_1.unperfect.gen.s \
	moving_i_2.unperfect.gen.s \
	moving_i_3.unperfect.gen.s \
	moving_barcode_dc31.unperfect.gen.s \
	moving_barcode_dc32.unperfect.gen.s \
	moving_barcode_dc33.unperfect.gen.s \
	moving_barcode_lexan1.unperfect.gen.s \
	moving_barcode_lexan2.unperfect.gen.s \
	moving_barcode_lexan3.unperfect.gen.s \
	moving_barcode_nerual1.unperfect.gen.s \
	moving_barcode_nerual2.unperfect.gen.s \

serve_sounds = \
	serve_g_left.snd.s \
	serve_g_right.snd.s \
	serve_i.snd.s \

win_sounds = \
	win_b.snd.s \
	win_e.snd.s \
	win_f.snd.s \
	win_h.snd.s \
	win_i_1.snd.s \
	win_i_2.snd.s \
	win_i_3.snd.s \
	win_i_4.snd.s \

lose_sounds = \
	lose_a.snd.s \
	lose_c.snd.s \
	lose_d.snd.s \
	lose_e.snd.s \
	lose_f.snd.s \
	lose_h.snd.s \
	lose_i_1.snd.s \
	lose_i_2.snd.s \
	endgame_a.snd.s \
	losegame_b.snd.s \
	wingame_e.snd.s \
	losegame_c.snd.s \

opponents_sprites = \
	$(sprites_unperfect) \
	$(names_unperfect) \
	$(serve_unperfect) \
	$(lose_unperfect) \
	$(win_unperfect) \
	$(special_unperfect) \

sprites = \
	$(sprites_pixel_perfect) \
	$(opponents_sprites)

sounds = \
	puck_hit.snd.s \
	crash.snd.s \
	puck.snd.s \
# Our code
# Order is important so that things are linked in the expected order
# in the various segments, and things needing to be aligned are aligned
shflpuck_SOURCES := \
	code/buffers.s \
	play_sound.gen.s \
	$(opponents_code) \
	$(serve_sounds) \
	$(win_sounds) \
	$(lose_sounds) \
	$(sprites) \
	$(sounds) \
	code/init.s \
	code/main.s \
	code/hz.s \
	code/mouse.s \
	code/crash_lines.s \
	code/draw_sprite.s \
	code/draw_score.s \
	code/plot.s \
	code/game_loop.s \
	code/load_data.s \
	code/sprites.s \
	code/mini_conio.s \
	code/net_serial.s \
	code/net_acia.s \
	code/net_z8530.s \
	../lib/fastirq.s \
	../lib/asm/hgr_tables.s \
	../lib/asm/hgr.s \
	../lib/asm/auxhgr_file.s \
	../lib/asm/hgr_mono40.s \
	../lib/asm/hgr_setup_mono40.s \
	../lib/extrazp.s \
	../lib/cntoabuf.s \
	../lib/platform_sleep.s \
	transform.s \
	$(assets_backgrounds) \
	$(levels_code) \
	splash.bg.gen.s \
	bar.snd.s \
	font.gen.s \
	code/bar.s \
	code/draw_text.s \
	pointer.pixel_perfect.gen.s \

shflpuck_CLCFLAGS = -t $(CC65_TARGET) -I ../lib -Cl \
	-vm -m shflpuck.map \
	--asm-include-dir $(shell pwd) \
	--asm-define MAX_COMPRESSED_DATA_SIZE=$(MAX_COMPRESSED_DATA_SIZE) \
	--asm-define DISABLE_DHGR \
	$(UNKILLABLE_CFLAG) \
	$(DEBUG_OPPONENT_CFLAG)

# Targets

all dist: \
	shufflepuck.po

gen_sprite: ../gen_sprite/gen_sprite.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_font: ../gen_font/gen_font.c
	gcc -g -O0 $(shell pkg-config --cflags SDL_image) $^ $(shell pkg-config --libs SDL_image) -o $@

gen_sound_player: ../sound_player_generator/gen_sound_player.c ../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -DENABLE_SLOWER=1 -g -O0 $< -o $@

gen_sound_clip: ../sound_player_generator/gen_sound_clip.c ../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -DENABLE_SLOWER=1 -g -O0 $< -o $@

gen_transform: generators/gen_transform.c generators/one-point-perspective.h
	gcc -g -O0 $< -o $@

transform.s: gen_transform
	./$< > $@

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
my%.pixel_perfect.gen.s: assets/my%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SDATA nomask
their%.pixel_perfect.gen.s: assets/their%.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SDATA
puck0.pixel_perfect.gen.s: assets/puck0.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SPLC
puck1.pixel_perfect.gen.s: assets/puck1.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SPLC
puck2.pixel_perfect.gen.s: assets/puck2.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SDATA
puck3.pixel_perfect.gen.s: assets/puck3.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SDATA
puck4.pixel_perfect.gen.s: assets/puck4.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SPLC
puck5.pixel_perfect.gen.s: assets/puck5.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SDATA
puck6.pixel_perfect.gen.s: assets/puck6.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SDATA
pointer.pixel_perfect.gen.s: assets/pointer.png gen_sprite
	./gen_sprite $< 255 pixel_perfect barcode
hand.pixel_perfect.gen.s: assets/hand.png gen_sprite
	./gen_sprite $< 255 pixel_perfect SDATA

$(opponents_sprites): %.unperfect.gen.s: assets/opponents/%.png
%.unperfect.gen.s: assets/opponents/%.png gen_sprite
	./gen_sprite $< 255 unperfect $(shell echo "$@"|sed 's/[a-z]*_\([a-z]\+\).*$$/\1/') big

play_sound.gen.s: gen_sound_player
	./gen_sound_player $(SAMPLING_HZ) LOWCODE > $@

$(sounds): %.snd.s: assets/sounds/%.wav
crash.snd.s: assets/sounds/crash.wav gen_sound_clip
	rm -f $<_raw
	sox $< -R -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw SDATA > $@
	rm $<_raw
puc%.snd.s: assets/sounds/puc%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -R -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw SPLC > $@
	rm $<_raw
bar.snd.s: assets/sounds/bar.wav gen_sound_clip
	rm -f $<_raw
	sox $< -R -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw barsnd > $@
	rm $<_raw
serve%.snd.s: assets/sounds/opponents/serve%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -R -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw $(shell echo "$@"|sed 's/[a-z]*_\([a-z]\).*$$/\1/') > $@
	rm $<_raw
win%.snd.s: assets/sounds/opponents/win%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -R -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw $(shell echo "$@"|sed 's/[a-z]*_\([a-z]\).*$$/\1/') > $@
	rm $<_raw
lose%.snd.s: assets/sounds/opponents/lose%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -R -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw $(shell echo "$@"|sed 's/[a-z]*_\([a-z]\).*$$/\1/') > $@
	rm $<_raw
end%.snd.s: assets/sounds/opponents/end%.wav gen_sound_clip
	rm -f $<_raw
	sox $< -R -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw $(shell echo "$@"|sed 's/[a-z]*_\([a-z]\).*$$/\1/') > $@
	rm $<_raw

font.gen.s: assets/fonts/font.png gen_font
	./gen_font $< 280 barcode

shflpuck.ld.cfg: shflpuck.ld.cfg.tmpl generators/gen_linker_config.sh
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
	shflpuck.ld.cfg

check_imports.sh: $(shflpuck_SOURCES) check_imports.sh
	cp -f check_imports.sh .
	./check_imports.sh code
	./check_imports.sh levels_data

$(BINARY_NAME): shflpuck.ld.cfg $(shflpuck_SOURCES)
	cl65 -g -Wl --dbgfile,shflpuck.dbg -Ln shflpuck.lbl $(CFLAGS) $(shflpuck_CLCFLAGS) \
		-C shflpuck.ld.cfg -o $@ $(shflpuck_SOURCES) \
		$(CC65_TARGET)-iobuf-0800.o $(CC65_TARGET)-integer-basic-compat.o

CLEANDISK = ../../disks/ProDOS_2_4_3.po
shufflepuck.po: $(BINARY_NAME) $(assets_zx02)
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ SHUFFLEPUCK
	java -jar ../../bin/ac.jar -p $@ SHFLPUCK.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar ../../bin/ac.jar -as $@ SHFLPUCK < $<; \
	for r in $(assets_zx02); do \
		java -jar ../../bin/ac.jar -p $@ $$(basename $$r | sed "s/\.zx02$///") BIN < $$r; \
	done; \

upload: all
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/ && \
	cp *.bin *.zx02 /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/shufflepuck/$(GAME_LANG)/
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/ && \
	cp *.po /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/disks/

gimp_setup:
	./gimp_setup.sh

dist: shufflepuck.po
	mkdir -p ../../dist/
	cp $< ../../dist/
