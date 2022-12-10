#include "simple_serial.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NCYCLES_PER_SEC 300U

static struct ser_params default_params = {
    SER_BAUD_9600,     /* Baudrate */
    SER_BITS_8,         /* Number of data bits */
    SER_STOP_1,         /* Number of stop bits */
    SER_PAR_NONE,       /* Parity setting */
    SER_HS_HW           /* Type of handshake to use */
};

int simple_serial_open(int slot, int baudrate) {
  extern char a2_ssc;
  int err;
  
  if ((err = ser_install(&a2_ssc)) != 0)
    return err;

  if ((err = ser_apple2_slot(slot)) != 0)
    return err;

  default_params.baudrate = baudrate;
  return ser_open (&default_params);
}

int simple_serial_close(void) {
  return ser_uninstall();
}

static long timeout_cycles = -1;
static int timeout_secs = -1;

void simple_serial_set_timeout(int timeout) {
  timeout_secs = timeout;
}

static void serial_timeout_init() {
  if (timeout_secs < 0)
    return;

  timeout_cycles = NCYCLES_PER_SEC * timeout_secs;
}

static int serial_timeout_reached(void) {
  if (timeout_cycles < 0)
    return 0;

  timeout_cycles--;

  return timeout_cycles == 0;
}

static void serial_timeout_reset(void) {
  timeout_cycles = -1;
}

static int __simple_serial_getc_with_timeout(int with_timeout) {
    char c;

    serial_timeout_init();
    while (ser_get(&c) == SER_ERR_NO_DATA){
      if (with_timeout && serial_timeout_reached()) {
        serial_timeout_reset();
        return EOF;
      }
    }

    serial_timeout_reset();

    return (int)c;
}

int simple_serial_getc_with_timeout(void) {
  return __simple_serial_getc_with_timeout(1);
}

char simple_serial_getc(void) {
  return (char)__simple_serial_getc_with_timeout(0);
}

static char *__simple_serial_gets_with_timeout(char *out, size_t size, int with_timeout) {
  int b;
  char c;
  size_t i = 0;

  if (size == 0) {
    return NULL;
  }

  while (i < size - 1) {
    b = __simple_serial_getc_with_timeout(with_timeout);
    if (b == EOF) {
      break;
    }
    c = (char)b;
    if (c == '\r') {
      /* ignore \r */
      continue;
    }
    if (c == '\n') {
      break;
    }
    out[i] = c;
    i++;
  }
  out[i] = '\0';

  return out;
}

char *simple_serial_gets_with_timeout(char *out, size_t size) {
  return __simple_serial_gets_with_timeout(out, size, 1);
}

char *simple_serial_gets(char *out, size_t size) {
  return __simple_serial_gets_with_timeout(out, size, 0);
}

static size_t __simple_serial_read_with_timeout(char *ptr, size_t size, size_t nmemb, int with_timeout) {
  int b;
  size_t i = 0;
  size_t tries = 0;

  if (size != 1) {
    /* unsupported */
    return 0;
  }

  while (i < (nmemb - 1)) {
    b = __simple_serial_getc_with_timeout(with_timeout);
    if (b == EOF){
      break;
    }
    ptr[i] = (char)b;
    i++;
  }

  return i;
}

size_t simple_serial_read_with_timeout(char *ptr, size_t size, size_t nmemb) {
  return __simple_serial_read_with_timeout(ptr, size, nmemb, 1);
}

size_t simple_serial_read(char *ptr, size_t size, size_t nmemb) {
  return __simple_serial_read_with_timeout(ptr, size, nmemb, 0);
}
