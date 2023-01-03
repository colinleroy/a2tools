SUBDIRS = src 
PROGS = src/a2recv/a2recv.bin \
				src/http-client/httpcli.bin \
				src/homecontrol-client/homectrl.bin \
				src/homecontrol-client/mtrcftch.bin \
				src/homecontrol-client/grphview.bin \

CLEANDISK = disks/basic-empty.dsk
DISK = disks/homectrl.dsk

.PHONY: all clean

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@ || exit; \
	done

disk: all
	cp $(CLEANDISK) $(DISK); \
	java -jar bin/ac.jar -p $(DISK) BASIC.SYSTEM SYS < bin/loader.system; \
	for prog in $(PROGS); do \
		java -jar bin/ac.jar -as $(DISK) $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
		cp $(DISK) ~/Documents/ADTPro-2.1.0/disks/; \
	done
