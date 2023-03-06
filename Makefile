SUBDIRS = src 
net_disk_PROGS = src/a2recv/a2recv.bin \
		 		src/a2send/a2send.bin \
				src/surl/surl.bin \
				src/stp/stp.bin \
				src/telnet/telnet.bin

homectrl_disk_PROGS = src/a2recv/a2recv.bin \
				src/homecontrol-client/homectrl.bin \
				src/homecontrol-client/grphview.bin \

mastodon_disk_PROGS = src/mastodon/mastodon.bin \
				src/mastodon/mastocli.bin \
				src/mastodon/mastowrite.bin \
				src/mastodon/mastoconf.bin \
				src/mastodon/mastoimg.bin

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
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

homectrl.dsk: $(homectrl_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -p $@ BASIC.SYSTEM SYS < bin/loader.system; \
	# java -jar bin/ac.jar -p $@ SRVURL TXT < src/homecontrol-client/SRVURL; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

mastodon.dsk: $(mastodon_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -p $@ MASTODON.SYSTEM SYS < bin/loader.system; \
	# java -jar bin/ac.jar -p $@ mastsettings TXT < src/mastodon/mastsettings; \
	# java -jar bin/ac.jar -p $@ clisettings TXT < src/mastodon/clisettings; \
	java -jar bin/ac.jar -d $@ BASIC; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
	done
	cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	cp $@ dist/; \

dist: clean all net.dsk homectrl.dsk mastodon.dsk

upload: all
	scp $(net_disk_PROGS) $(homectrl_disk_PROGS) $(mastodon_disk_PROGS) diskstation.lan:/volume1/a2repo/apple2/
