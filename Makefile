SUBDIRS = src 
net_disk_PROGS = src/a2recv/a2recv.bin \
		 		src/a2send/a2send.bin \
				src/surl/surl.bin \
				src/stp/stp.bin \

homectrl_disk_PROGS = src/a2recv/a2recv.bin \
				src/homecontrol-client/homectrl.bin \
				src/homecontrol-client/grphview.bin \

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
		cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	done

homectrl.dsk: $(homectrl_disk_PROGS)
	cp $(CLEANDISK) $@; \
	java -jar bin/ac.jar -p $@ BASIC.SYSTEM SYS < bin/loader.system; \
	for prog in $^; do \
		java -jar bin/ac.jar -as $@ $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
		cp $@ ~/Documents/ADTPro-2.1.0/disks/; \
	done

upload: clean all
	scp $(net_disk_PROGS) $(homectrl_disk_PROGS) diskstation.lan:/volume1/a2repo/
