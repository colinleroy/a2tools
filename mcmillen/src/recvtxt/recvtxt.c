#include <stdio.h>
#include <stdlib.h>
#include "simple_serial.h"
#include <string.h>
#include <dbg.h>

#define BUF_SIZE 255


int main(void) {
  int t, total_lines = 0, tm;
  char *buf = malloc(BUF_SIZE);
  char *large_buf = malloc(4096);

  simple_serial_open(2, SER_BAUD_9600);

  if (simple_serial_gets(buf, BUF_SIZE) != NULL) {
    printf("Read '%s'\n", buf);
  }

  tm = atoi(buf);
  printf("setting read timeout to %d\n", tm);
  simple_serial_set_timeout(tm);

  printf("Reading buffer without timeout\n");
  t = simple_serial_read(large_buf, sizeof(char), 4096);

  printf("Read buffer of %d bytes\n", t);

  printf("Reading buffer with timeout\n");
  t = simple_serial_read_with_timeout(large_buf, sizeof(char), 4096);

  printf("Read buffer of %d bytes\n", t);

  free(buf);
  free(large_buf);

  simple_serial_close();
  exit(0);
}
