#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>

#include "simple_serial.h"
#include "extended_conio.h"

extern FILE *ttyfp;

#define OFFSET 0x40

int rate = 115200/(1+8+1);

int main(int argc, char *argv[]) {
  int num = 0, r;
  unsigned char c;
  struct timeval samp_start, samp_end;
  unsigned long secs;
  unsigned long microsecs;
  unsigned long elapsed;
  int max = 0;
  int i;

  if (simple_serial_open() != 0) {
    printf("Can't open serial\n");
    exit(1);
  }
  if (argc < 2) {
    printf("Usage: %s [audio]\n", argv[0]);
    exit(1);
  }
  if (argc == 3) {
    rate = atoi(argv[2]);
  }
  FILE *fp = fopen(argv[1],"rb");

  while ((i = fgetc(fp)) != EOF) {
    if (i > max)
      max = i;
  }

  if (max == 0) {
    max = 255;
  }

  rewind(fp);
  printf("Max volume: %d\n", max);

  gettimeofday(&samp_start, 0);

  /* Pulses tester */
  for (i = 0; i < 32; i++) {
    simple_serial_putc(i+OFFSET);
  }

  while (1) {
    i = fgetc(fp);
    if (i == EOF) {
      break;
    }

    i = OFFSET + i*31/max;

    simple_serial_putc(i);
    if (num == rate) {
      gettimeofday(&samp_end, 0);
      secs      = (samp_end.tv_sec - samp_start.tv_sec)*1000000;
      microsecs = samp_end.tv_usec - samp_start.tv_usec;
      elapsed   = secs + microsecs;
      printf("%d samples in %d µs\n", num, elapsed);
      num = 0;
      gettimeofday(&samp_start, 0);
    }
    num++;
  }
  //simple_serial_putc(32+OFFSET);
  fclose(fp);
}
