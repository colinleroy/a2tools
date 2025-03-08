SRC_DIR=../../
CC65_TARGET = apple2enh
serial_hw := z8530
suffix := iigs
iigs_suffix := -iigs
iigs_CFLAGS := -DIIGS --asm-define IIGS
STREAMER_VER := 65c02
trgt = apple2gs
upload_subdir := /iigs
EMBED_RBROWSER := \
	$(SRC_DIR)/radio-browser.c \
	$(SRC_DIR)/../lib/surl/asm/surl_get_json.s
EXTERNAL_RBROWSER :=

include ../Makefile
