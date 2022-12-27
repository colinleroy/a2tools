#ifdef __CC65__
#include <serial.h>
#else
#include <termios.h>
#endif

#include <stdlib.h>

/* Setup */
#ifdef __CC65__
int simple_serial_open(int slot, int baudrate);
int simple_serial_close(void);
#else
int simple_serial_open(const char *tty, int baudrate);
int simple_serial_close(void);
#endif
void simple_serial_set_timeout(int timeout);

/* Input */
char simple_serial_getc(void);
int simple_serial_getc_with_timeout(void);

char *simple_serial_gets(char *out, size_t size);
char *simple_serial_gets_with_timeout(char *out, size_t size);

size_t simple_serial_read(char *ptr, size_t size, size_t nmemb);
size_t simple_serial_read_with_timeout(char *ptr, size_t size, size_t nmemb);

/* Output */
int simple_serial_putc(char c);
int simple_serial_puts(char *buf);
int simple_serial_printf(const char* format, ...);