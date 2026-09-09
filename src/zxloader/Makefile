all: loader.system zxloader.system

zxloader.system: loader.s zx02_direct.s
	cl65 --debug-info -Wl --dbgfile,"zxloader.dbg" -vm -m zxloader.map \
	--asm-define ENABLE_DECOMPRESSOR \
	-t apple2enh -C loader.cfg -o $@ loader.s

loader.system: loader.s
	cl65 --debug-info -Wl --dbgfile,"loader.dbg" -vm -m loader.map \
	-t apple2enh -C loader.cfg -o $@ loader.s

clean:
	rm -f *.map *.dbg *.system
