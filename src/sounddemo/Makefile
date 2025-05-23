ifdef OLDII
STREAMER_VER := 6502
CC65_TARGET := apple2
TARGET_PLATFORM := apple2p
else
STREAMER_VER := 65c02
CC65_TARGET := apple2enh
TARGET_PLATFORM := apple2c0
endif

CLEANDISK = ../../disks/ProDOS_2_4_3.po

#Sound generator sampling frequency
SAMPLING_HZ := 8000

sounds = \
	countdown_$(SAMPLING_HZ).snd.s

sounddemo_SOURCES := \
	main.s \
	play_sound.gen.s \
	$(sounds)

all: \
	sounddemo.po

sounddemo_CLCFLAGS = -t $(CC65_TARGET) -Cl -C ../../config/apple2enh-aligndata.cfg \
	-vm -m sounddemo.map --start-addr 0x803

#Build the sound player with the correct definitions
gen_sound_player: ../sound_player_generator/gen_sound_player.c ../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -DENABLE_SLOWER -g -O0 $< -o $@

#Build the sound clip generator with the correct definitions
gen_sound_clip: ../sound_player_generator/gen_sound_clip.c ../sound_player_generator/sound.h
	gcc -DCPU_$(STREAMER_VER) -DENABLE_SLOWER -g -O0 $< -o $@

play_sound.gen.s: gen_sound_player
	./gen_sound_player $(SAMPLING_HZ) > $@

$(sounds): %.snd.s: %.wav
%.snd.s: %.wav gen_sound_clip
	rm -f $<_raw
	sox $< -b 8 -e unsigned-integer -t raw $<_raw channels 1 rate -v $(SAMPLING_HZ)
	./gen_sound_clip $(SAMPLING_HZ) $<_raw > $@
	rm $<_raw

clean:
	rm -f *.bin *.o \
	gen_sound_clip \
	gen_sound_player \
	*.gen.s \
	*.gen.inc \
	*.snd.s \
	*.dbg *.lbl *.map \
	*.po

sounddemo.bin: $(sounddemo_CL65SOURCES) $(sounddemo_SOURCES)
	cl65 -g -Wl --dbgfile,sounddemo.dbg -Ln sounddemo.lbl $(CFLAGS) $(sounddemo_CLCFLAGS) -o $@ $^

sounddemo.po: sounddemo.bin
	cp $(CLEANDISK) $@; \
	java -jar ../../bin/ac.jar -n $@ SNDDEMO
	java -jar ../../bin/ac.jar -p $@ SNDDEMO.SYSTEM SYS < ../../bin/loader.system; \
	java -jar ../../bin/ac.jar -d $@ BASIC.SYSTEM; \
	java -jar ../../bin/ac.jar -as $@ SNDDEMO < $<;

test: sounddemo.po
	mame $(TARGET_PLATFORM) -window -flop1 $<

upload: sounddemo.po
	test -d /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/snddemo/ || \
	mkdir -p /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/snddemo/ && \
	cp sounddemo.bin /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/snddemo/SNDDEMO.BIN && \
	cp ../../bin/loader.system /run/user/1000/gvfs/smb-share\:server\=diskstation.lan\,share\=a2repo/apple2$(upload_subdir)/snddemo/SNDDEMO.SYSTEM.SYS
