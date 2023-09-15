#ifndef __simple_serial_h
#define __simple_serial_h
#include <stdlib.h>
#ifdef __CC65__
#include <serial.h>
#else
#include <termios.h>
#endif

#ifndef __CC65__
#define __fastcall__
#endif

#define SIMPLE_SERIAL_BUF_SIZE 512

/* Setup */
#ifdef __CC65__
int __fastcall__ simple_serial_open(void);
int __fastcall__ simple_serial_close(void);
void __fastcall__ simple_serial_flush(void);
#define simple_serial_putc(c) ser_put(c)
#else
int simple_serial_open(void);
int simple_serial_close(void);
void simple_serial_flush(void);
unsigned char __fastcall__ simple_serial_putc(char c);
#endif

/* Input */
char __fastcall__ simple_serial_getc(void);
int __fastcall__ simple_serial_getc_with_timeout(void);
int __fastcall__ simple_serial_getc_immediate(void);

char * __fastcall__ simple_serial_gets(char *out, size_t size);

void __fastcall__ simple_serial_read(char *ptr, size_t nmemb);

/* Output */
void __fastcall__ simple_serial_puts(char *buf);
void __fastcall__ simple_serial_write(char *ptr, size_t nmemb);

void simple_serial_printf(const char* format, ...);
#endif
