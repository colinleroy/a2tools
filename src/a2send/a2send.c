/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#ifndef __CC65__
#include <libgen.h>
#else
#include "extended_conio.h"
#endif
#include <stdlib.h>
#include <string.h>
#include "simple_serial.h"

#define DATA_SIZE 16384

static void wait_for_receiver(void) {
  char buf[128];

  printf("Waiting for receiver...\n");
  do {
    if (simple_serial_gets(buf, sizeof(buf)) != NULL) {
      if(strchr(buf, '\n'))
        *strchr(buf, '\n') = '\0';
      if (!strcasecmp(buf, "READY"))
        break;
    }
#ifndef __CC65__
    usleep(50 * 1000);
#endif
  }
  while (1);
#ifndef __CC65__
  sleep(1);
#endif
}

#ifdef __CC65__
static unsigned long get_filesize(char *file) {
  struct dirent *d;
  DIR *dir;
  unsigned long size = 0;

  dir = opendir(".");
  while (d = readdir(dir)) {
    if (!strcmp(file, d->d_name)) {
      size = d->d_size;
      break;
    }
  }

  closedir(dir);

  if (size == 0) {
    printf("Can't find %s\n", file);
    exit(1);
  }
  return size;
}
#else
static unsigned long get_filesize(char *file) {
  struct stat statbuf;

  if (stat(file, &statbuf) < 0) {
    printf("Can't stat %s\n", file);
    exit(1);
  }
  
  return statbuf.st_size;
}
#endif

int main(int argc, char **argv) {
  FILE *fp;
  char *filename;
  char *filetype;
  char c;
  int count = 0;
  char buf[128];
  char start_addr[2];
  int cur_file = 2;
  unsigned long filesize;

#ifdef __CC65__
  if (simple_serial_open(2, SER_BAUD_9600, 1) < 0) {
    exit(1);
  }

send_again:
  printf("File to send: ");
  cgets(buf, sizeof(buf));

  filename = buf;

  filesize = get_filesize(argv[cur_file]);

  fp = fopen(filename,"r");
  if (fp == NULL) {
    printf("Can't open %s\n", argv[cur_file]);
    exit(1);
  }

  if (strchr(filename, '.') != NULL) {
    filetype = strchr(filename, '.') + 1;
    *(strchr(filename, '.')) = '\0';
  } else {
    filetype = "TXT";
  }
  if (!strcmp(filetype, "system")) {
    char *tmp = malloc(32);
    filetype = "SYS";
    sprintf(tmp, "%s.system", filename);
    free(filename);
    filename = tmp;
  }
  
  if (!strcmp(filetype, "TXT")) {
    _filetype = PRODOS_T_TXT;
    _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  } else if (!strcmp(filetype, "BIN")) {
    _filetype = PRODOS_T_BIN;
  } else if (!strcmp(filetype, "SYS")) {
    _filetype = PRODOS_T_SYS;
  }

#else

  if (argc < 3) {
    printf("Usage: %s [output tty] [file(s) to send]\n", argv[0]);
    exit(1);
  }

  if (simple_serial_open(argv[1], B9600, 1) < 0) {
    exit(1);
  }

send_again:

  fp = fopen(argv[cur_file],"r");
  if (fp == NULL) {
    printf("Can't open %s\n", argv[cur_file]);
    exit(1);
  }
  filesize = get_filesize(argv[cur_file]);

  filename = basename(argv[cur_file]);
  if (strchr(filename, '.') != NULL) {
    filetype = strchr(filename, '.') + 1;
    *(strchr(filename, '.')) = '\0';
  } else {
    filetype = "TXT";
  }
  if (!strcmp(filetype, "system")) {
    filetype = "SYS";
  }
#endif

  /* Send filename */
  simple_serial_printf("%s\n", filename);
  printf("Filename sent:    %s\n", filename);

  /* Send filetype */
  simple_serial_printf("%s\n", filetype);
  printf("Filetype sent:    %s\n", filetype);

#ifndef __CC65__
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
#endif

  /* Send data length */
  simple_serial_printf("%ld\n", filesize - ftell(fp));

  printf("Data length sent: %ld\n", filesize - ftell(fp));

  if (!strcasecmp(filetype, "BIN")) {
    simple_serial_printf("%02x%02x\n", start_addr[0], start_addr[1]);
    printf("Start address:    %02x%02x\n", start_addr[0], start_addr[1]);
  }

  while(fread(&c, 1, 1, fp) > 0) {
    simple_serial_putc(c);
    count++;
    
    if (count % 512 == 0) {
      printf("\rWrote %d bytes...", count);
      fflush(stdout);
    }
    
    if (count % DATA_SIZE == 0) {
      /* Wait for Apple // */
      printf("\n");
      wait_for_receiver();
    }
  }

  fclose(fp);
  
  printf("\n");
  wait_for_receiver();

#ifdef __CC65__
  free(filename);
#endif
  if (cur_file < argc -1) {
    cur_file++;
    count = 0;
    printf("\n");
    goto send_again;
  }
  simple_serial_close();
  printf("Wrote %d bytes.\n", count);
  exit(0);
}
