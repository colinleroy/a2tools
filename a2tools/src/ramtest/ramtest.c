#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#endif
#include "extended_conio.h"

int main(void) {
  unsigned int total = 0;
  while (1) {
    char *buf;

    printf("Heap avail %u, max block %u\n", _heapmemavail(), _heapmaxavail());
    cgetc();  
    buf = malloc(1024);
    if (buf != NULL) {
      memset(buf, 0, 1024);
      total += 1024;
      printf("Allocated %u\n", total);
    } else {
      printf("Can't malloc\n");
      cgetc();
      break;
    }
  }
  exit (0);
}
