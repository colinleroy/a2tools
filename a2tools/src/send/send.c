#include <stdio.h>
#include <stdlib.h>
#include "simple_serial.h"
#include <string.h>
#include <dbg.h>
#include <errno.h>
#include <apple2.h>
#include <conio.h>

#define BUF_SIZE 255

/**********************
 * NON FUNCTIONAL NOW
 */////////////////////
int main(void) {
//  int r, w, exit_code = 0;
  int exit_code = 0, e;
  char c;
  int j;
  char *buf = malloc(BUF_SIZE);

  printf("Opening serial port\n");
  simple_serial_open(2, SER_BAUD_9600);

  clrscr();
  gotoxy(0,0);
  simple_serial_set_timeout(10);
  while (1) {
    printf("> ");
    if (simple_serial_gets_with_timeout(buf, BUF_SIZE) != NULL) {
      char i;
      printf("%s\n", buf);

      for (i = 'a'; i < 'h'; i++) {
        ser_put(i);
      }
      ser_put('\n');

      for (j = 0; j < 5000; j++);
    }
  }

  simple_serial_close();
  exit(exit_code);
}
