#include "simple_serial.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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

int simple_serial_close() {
  return ser_uninstall();
}

static char ser_buf[255];

char *simple_serial_gets() {
  char *out = NULL;
  char c;
  size_t i = 0;

  while (i < sizeof(ser_buf) - 1) {
    while (ser_get(&c) == SER_ERR_NO_DATA);
    if (c == '\r') {
      /* ignore \r */
      continue;
    }
    if (c == '\n') {
      break;
    }
    ser_buf[i] = c;
    i++;
  }
  ser_buf[i] = '\0';

  out = malloc(i + 1);
  memcpy(out, ser_buf, i + 1);
  
  return out;
}
