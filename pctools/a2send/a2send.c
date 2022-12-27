#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include "simple_serial.h"

#define DATA_SIZE 16384

int main(int argc, char **argv) {
  FILE *fp;
  char *filename;
  char *filetype;
  char c;
  int count = 0;
  char buf[128];
  char start_addr[2];
  struct stat statbuf;

  if (argc < 3) {
    printf("Usage: %s [file] [output tty]\n", argv[0]);
    exit(1);
  }

  if (stat(argv[1], &statbuf) < 0) {
    printf("Can't stat %s\n", argv[1]);
    exit(1);
  }
  
  fp = fopen(argv[1],"r");
  if (fp == NULL) {
    printf("Can't open %s\n", argv[1]);
    exit(1);
  }
  
  filename = basename(argv[1]);
  if (strchr(filename, '.') != NULL) {
    filetype = strchr(filename, '.') + 1;
    *(strchr(filename, '.')) = '\0';
  } else {
    filetype = "TXT";
  }
  if (!strcmp(filetype, "system")) {
    filetype = "SYS";
  }

  if (simple_serial_open(argv[2], B9600) < 0) {
    exit(1);
  }

  /* Send filename */
  simple_serial_printf("%s\n", filename);
  printf("Filename sent:    %s\n", filename);

  /* Send filetype */
  simple_serial_printf("%s\n", filetype);
  printf("Filetype sent:    %s\n", filetype);

  if (!strcasecmp(filetype, "BIN")) {
    char buf[58];
    fread(buf, 1, 58, fp);
    if (buf[0] == 0x00 && buf[1] == 0x05
     && buf[2] == 0x16 && buf[3] == 0x00) {
      printf("AppleSingle file detected, skipping header\n");
      start_addr[0] = buf[56];
      start_addr[1] = buf[57];
    } else {
      rewind(fp);
    }
  }

  /* Send data length */
  simple_serial_printf("%ld\n", statbuf.st_size - ftell(fp));

  printf("Data length sent: %ld\n", statbuf.st_size - ftell(fp));

  if (!strcasecmp(filetype, "BIN")) {
    simple_serial_printf("%02x%02x\n", start_addr[0], start_addr[1]);
    printf("Start address sent:    %02x%02x\n", start_addr[0], start_addr[1]);
  }
  while(fread(&c, 1, 1, fp) > 0) {
    simple_serial_putc(c);
    count++;
    
    if (count % 512 == 0) {
      printf("Wrote %d bytes...\n", count);
    }
    
    if (count % DATA_SIZE == 0) {
      /* Wait for Apple // */
      printf("Waiting for the Apple //c...\n");

      do {
        if (simple_serial_gets(buf, sizeof(buf)) != NULL) {
          if(strchr(buf, '\n'))
            *strchr(buf, '\n') = '\0';
          printf("Received '%s'\n", buf);
          if (!strcasecmp(buf, "READY"))
            break;
        }
        usleep(50 * 1000);
      }
      while (1);

      printf("Resuming\n");
      sleep(3);
    }
  }

  fclose(fp);
  simple_serial_close();
  printf("Wrote %d bytes.\n", count);
  exit(0);
}