SOURCES := simple_serial.c \
	extended_conio.c \
	extended_string.c \
	bool_array.c \
	array_sort.c \
	bool_array_test.c

DEPS := $(topdir)../../drivers/a2.ssc.o

CC65_TARGET = apple2
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) 
LDFLAGS = -t $(CC65_TARGET)

.PHONY: all clean
all: $(SOURCES:.c=.o) \
	bool_array_test

clean:
	rm -f $(PROGRAM) *.o *.map

bool_array_test: bool_array.o extended_conio.o bool_array_test.o
	$(CC) $(LDFLAGS) -o $@ $^ $(DEPS)
