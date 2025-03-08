SRC_DIR=../../
TARGET := apple2enh
USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD
serial_hw_SOURCES := \
	$(SRC_DIR)/../lib/serial/z8530/common.s \
	$(SRC_DIR)/../lib/serial/z8530/control.s \
	$(SRC_DIR)/../lib/serial/z8530/no_irq.s \
	$(SRC_DIR)/../lib/fastirq-iigs.s
iigs_CFLAGS := -DIIGS --asm-define IIGS
suffix := iigs
CPU=65c02

include ../Makefile
