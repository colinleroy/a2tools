#include "simple_serial.h"

int simple_serial_read_no_irq(char *buffer, size_t len) {
  char c;
  while (len--) {
    c = simple_serial_getc_with_timeout();
    if (c == EOF) {
      return EOF;
    }
    *buffer++ = c;
  }
  return 0;
}
