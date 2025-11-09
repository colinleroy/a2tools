QT_EMU_SRC = \
	qt-emu.c \
	../lib/serial/c/simple_serial.c \
	../lib/serial/c/simple_serial_io.c \
	../lib/serial/c/serial_control.c \
	../lib/strtrim.c \
	../lib/extended_conio.c \
	../lib/c/clrzone.c \

QT_EMU_CFLAGS = \
	-DCONF_FILE_PATH=\"/etc/qt-emu.cfg\"

qt-emu:	$(QT_EMU_SRC)
	gcc $(QT_EMU_CFLAGS) -I . -I ../lib $^ -o qt-emu
