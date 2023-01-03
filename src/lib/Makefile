SOURCES := simple_serial.c \
	simple_em.c \
	extended_conio.c \
	extended_string.c \
	bool_array.c \
	array_sort.c \
	bool_array_test.c \
	slist.c \
	bigint.c \
	file_sorter.c \
	bfs.c \

DEPS :=

CC65_TARGET = apple2enh
CC      = cl65
CFLAGS  = -t $(CC65_TARGET)
LDFLAGS = -t $(CC65_TARGET)

.PHONY: all clean
all: $(SOURCES:.c=.o) \
	bat.bin

clean:
	rm -f $(PROGRAM) *.o *.map

bat.bin: bool_array.o extended_conio.o bool_array_test.o
	$(CC) $(LDFLAGS) -o $@ $^ $(DEPS)
	
