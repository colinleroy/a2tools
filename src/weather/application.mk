################################################################
# COMPILE FLAGS

# reserved memory for graphics
# LDFLAGS += -Wl -D,__RESERVED_MEMORY__=0x2000

LDFLAGS += --start-addr 0x4000 --ld-args -D,__HIMEM__=0xBF00 -m $(PROGRAM).map

ifeq ($(CURRENT_TARGET), apple2)
	CFLAGS += -DAPPLE2
endif
