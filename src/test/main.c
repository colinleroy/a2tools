#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <conio.h>
#include "simple_serial.h"

static char readbuf[2000];

int main(int argc, char *argv[]) {
  printf("ready\n");
//  cgetc();
  simple_serial_open(2, SER_BAUD_9600);
  simple_serial_read(readbuf, 2000);

  printf("done\n");
  cgetc();
  return 0;
}
