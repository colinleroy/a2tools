#ifndef __simple_serial_h
#define __simple_serial_h
#include <stdlib.h>
#include <unistd.h>
#ifdef __CC65__
#include <serial.h>
#else
#include <termios.h>
#endif

#ifndef __CC65__
#define __fastcall__
#endif

#define SIMPLE_SERIAL_BUF_SIZE 512

#ifdef IIGS
#define PRINTER_SER_SLOT 1
#define MODEM_SER_SLOT 0
#else
#define PRINTER_SER_SLOT 1
#define MODEM_SER_SLOT 2
#endif
/* Setup */

char simple_serial_settings_io(const char *path, char *mode);

void __fastcall__ simple_serial_set_speed(int b);
void __fastcall__ simple_serial_set_flow_control(unsigned char fc);

void __fastcall__ simple_serial_set_parity(unsigned int p);
void __fastcall__ simple_serial_dtr_onoff(unsigned char on);
void __fastcall__ simple_serial_acia_onoff(unsigned char slot_num, unsigned char on);

#ifdef __CC65__
char __fastcall__ simple_serial_open(void);
char __fastcall__ simple_serial_open_printer(void);
char __fastcall__ simple_serial_close(void);
void __fastcall__ simple_serial_flush(void);
void __fastcall__ simple_serial_set_irq(unsigned char on);
void __fastcall__ simple_serial_configure(void);
void __fastcall__ simple_serial_setup_no_irq_regs(void);
unsigned char __fastcall__ serial_read_byte_no_irq(void);
void __fastcall__ serial_putc_direct(unsigned char c);
void __fastcall__ simple_serial_read(char *ptr, size_t nmemb);

#define tty_speed_to_str(speed)        \
  ((speed == SER_BAUD_2400) ? "2400":  \
   (speed == SER_BAUD_4800) ? "4800":  \
   (speed == SER_BAUD_9600) ? "9600":  \
   (speed == SER_BAUD_19200)? "19200": \
   (speed == SER_BAUD_57600)? "57600":"115200")

#define simple_serial_putc serial_putc_direct

#else
int simple_serial_open(void);
int simple_serial_open_printer(void);
int simple_serial_close(void);
void simple_serial_flush(void);
#define simple_serial_configure()
#define simple_serial_setup_no_irq_regs()
unsigned char __fastcall__ simple_serial_putc(char c);
char *tty_speed_to_str(int speed);
void __fastcall__ simple_serial_read(char *ptr, size_t nmemb);
#endif

/* Input */
char __fastcall__ simple_serial_getc(void);
int __fastcall__ simple_serial_getc_with_timeout(void);
int __fastcall__ simple_serial_getc_immediate(void);

char * __fastcall__ simple_serial_gets(char *out, size_t size);

/* Output */
void __fastcall__ simple_serial_puts(const char *buf);
void __fastcall__ simple_serial_write(const char *ptr, size_t nmemb);

void simple_serial_printf(const char* format, ...);
#endif
