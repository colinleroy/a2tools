all: zxloader.system

zxloader.system: zxloader.s zx02_direct.s
	cl65 --debug-info -Wl --dbgfile,"zxloader.dbg" -vm -m zxloader.map \
	-C zxloader.cfg -o $@ zxloader.s

clean:
	rm -f *.map *.dbg *.system
