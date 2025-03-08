SRC_DIR=../../
serial_hw_SOURCES := $(SRC_DIR)/../lib/serial/z8530/common.s $(SRC_DIR)/../lib/serial/z8530/no_irq.s $(SRC_DIR)/../lib/serial/z8530/control.s
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := iigs
iigs_suffix := -iigs
USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD
CC65_TARGET := apple2enh

include ../Makefile
