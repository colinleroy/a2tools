#include <stdio.h>
#include <stdlib.h>
#include "simple_serial.h"
#include <string.h>
#include <dbg.h>
#include <errno.h>
#include <apple2.h>
#include <conio.h>

#define BUF_SIZE 255

int main(void) {
  int exit_code = 0;
  char *buf = malloc(BUF_SIZE);

  printf("Opening serial port\n");
  simple_serial_open(2, SER_BAUD_9600);

  clrscr();
  gotoxy(0,0);
  simple_serial_set_timeout(10);
  while (1) {
    printf("> ");
    if (simple_serial_gets_with_timeout(buf, BUF_SIZE) != NULL) {

      printf("%s", buf);

      simple_serial_puts(buf);
    }
  }

  simple_serial_close();
  exit(exit_code);
}
