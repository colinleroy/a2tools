SUBDIRS = drivers src
PROGS = src/recvtxt/recvtxt.bin
DISK = disks/adtpro.dsk

.PHONY: all clean

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@; \
	done

disk: all
	for prog in $(PROGS); do \
		java -jar bin/ac.jar -d $(DISK) $$(basename $$prog | sed "s/\.bin$///"); \
		java -jar bin/ac.jar -d $(DISK) $$(basename $$prog | sed "s/\.bin$///").SYSTEM; \
		java -jar bin/ac.jar -as $(DISK) $$(basename $$prog | sed "s/\.bin$///") < $$prog; \
		java -jar bin/ac.jar -p $(DISK) $$(basename $$prog | sed "s/\.bin$///").SYSTEM SYS < bin/loader.system; \
		cp $(DISK) ~/Documents/ADTPro-2.1.0/disks/; \
	done
