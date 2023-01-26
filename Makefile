SUBDIRS = src 
net_disk_PROGS = src/a2recv/a2recv.bin \
		 		src/a2send/a2send.bin \
				src/surl/surl.bin \
				src/stp/stp.bin \
				src/telnet/telnet.bin

homectrl_disk_PROGS = src/a2recv/a2recv.bin \
				src/homecontrol-client/homectrl.bin \
				src/homecontrol-client/grphview.bin \

mastapple_disk_PROGS = src/mastodon/mastodon.bin \
				src/mastodon/mastocli.bin \
				src/mastodon/mastowrite.bin \
				src/mastodon/mastoconf.bin

CLEANDISK = disks/basic-empty.dsk

.PHONY: all clean

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

net.dsk: $(net_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -p $@ BASIC.SYSTEM SYS < bin/loader.system; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	java -jar bin/ac.jar -p $@ telnet.system SYS < bin/loader.system; \
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \

homectrl.dsk: $(homectrl_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -p $@ BASIC.SYSTEM SYS < bin/loader.system; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
		cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	done

mastapple.dsk: $(mastapple_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -p $@ MASTODON.SYSTEM SYS < bin/loader.system; \
	java -jar bin/ac.jar -p $@ mastsettings TXT < src/mastodon/mastsettings; \
	java -jar bin/ac.jar -p $@ clisettings TXT < src/mastodon/clisettings; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
		cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	done

upload: all
	scp $(net_disk_PROGS) $(homectrl_disk_PROGS) $(mastapple_disk_PROGS) diskstation.lan:/volume1/a2repo/
