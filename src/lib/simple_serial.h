#ifdef __CC65__
#include <serial.h>
#else
#include <termios.h>
#endif

#ifndef __CC65__
#define __fastcall__
#endif


#include <stdlib.h>

/* Setup */
#ifdef __CC65__
int __fastcall__ simple_serial_open(int slot, int baudrate);
int __fastcall__ simple_serial_close(void);
void __fastcall__ simple_serial_flush(void);
#else
int simple_serial_open(void);
int simple_serial_close(void);
void simple_serial_flush(void);
#endif

/* Input */
char __fastcall__ simple_serial_getc(void);
int __fastcall__ simple_serial_getc_with_timeout(void);
int __fastcall__ simple_serial_getc_immediate(void);

char * __fastcall__ simple_serial_gets(char *out, size_t size);

void __fastcall__ simple_serial_read(char *ptr, size_t nmemb);

/* Output */
int __fastcall__ simple_serial_putc(char c);
void __fastcall__ simple_serial_puts(char *buf);
void simple_serial_printf(const char* format, ...);
void __fastcall__ simple_serial_write(char *ptr, size_t nmemb);

/* Status */
void __fastcall__ simple_serial_set_activity_indicator(char enabled, int x, int y);
