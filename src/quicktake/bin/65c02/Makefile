SRC_DIR=../../
TARGET := apple2enh
USE_LANGCARD := -DSURL_TO_LANGCARD --asm-define SURL_TO_LANGCARD
serial_hw_SOURCES := \
	$(SRC_DIR)/../lib/serial/acia/common.s \
	$(SRC_DIR)/../lib/serial/acia/control.s \
	$(SRC_DIR)/../lib/serial/acia/no_irq.s \
	$(SRC_DIR)/../lib/fastirq.s
iigs_FLAGS :=
suffix := 65c02
CPU=65c02

include ../Makefile
