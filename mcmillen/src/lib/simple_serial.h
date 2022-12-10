#include <serial.h>
#include <conio.h>

int simple_serial_open(int slot, int baudrate);

int simple_serial_close();

char *simple_serial_gets();
