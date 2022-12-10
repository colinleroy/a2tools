#include <serial.h>
#include <stdlib.h>
#include <conio.h>

int simple_serial_open(int slot, int baudrate);

int simple_serial_close(void);

void simple_serial_set_timeout(int timeout);

char simple_serial_getc(void);
int simple_serial_getc_with_timeout(void);

char *simple_serial_gets(char *out, size_t size);
char *simple_serial_gets_with_timeout(char *out, size_t size);

size_t simple_serial_read(char *ptr, size_t size, size_t nmemb);
size_t simple_serial_read_with_timeout(char *ptr, size_t size, size_t nmemb);
