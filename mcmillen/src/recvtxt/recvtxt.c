#include <stdio.h>
#include <stdlib.h>
#include "simple_serial.h"
#include <string.h>
#include <dbg.h>

#define HEAD 0
#define N_KNOTS 10


int main(void) {
  int t,total_lines = 0;

  simple_serial_open(2, SER_BAUD_9600);

  for (t = 0; t < 10; t++) {
    char *str = simple_serial_gets();
    printf("Read '%s'\n", str);
    free(str);
  }
  simple_serial_close();
  cgetc();
  exit(0);
}
