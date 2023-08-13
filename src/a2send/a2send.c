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
#include <apple2enh.h>
#endif
#include "cgets.h"
#include <stdlib.h>
#include <string.h>
#include "get_filedetails.h"
#include "simple_serial.h"

#define DATA_SIZE 16384
#define BUFSIZE 32

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
int main(void) {
#else
int main(int argc, char **argv) {
#endif
  FILE *fp;
  char *filename;
  char *filetype;
  char c;
  int count = 0;
#ifdef __CC65__
  char buf[128];
  char *remote_filename, *path;
#endif
  char start_addr[2] = {0, 0};
  int cur_file = 1;
  unsigned long filesize;
  unsigned char type;
  unsigned auxtype;

#ifdef __CC65__
  if (simple_serial_open(2, SER_BAUD_9600) < 0) {
    exit(1);
  }

  printf("File to send: ");
  cgets(buf, sizeof(buf));

  path = buf;
  if (strchr(path, '\n')) {
    *strchr(path, '\n') = '\0';
  }
  if (get_filedetails(path, &filename, &filesize, &type, &auxtype) < 0) {
    printf("Can't get %s details\n", path);
    exit(1);
  }

  /* We want to send raw */
  _filetype = PRODOS_T_TXT;
  _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  
  fp = fopen(path,"r");
  if (fp == NULL) {
    printf("Can't open %s\n", path);
    exit(1);
  }
  
  if (!strchr(filename, '.')) {
    remote_filename = malloc(BUFSIZE);
    if (type == PRODOS_T_SYS) {
      snprintf(remote_filename, BUFSIZE, "%s.SYSTEM", filename);
    } else if (type == PRODOS_T_BIN) {
      snprintf(remote_filename, BUFSIZE, "%s.BIN", filename);
    } else {
      snprintf(remote_filename, BUFSIZE, "%s.TXT", filename);
    }
  } else {
    remote_filename = strdup(filename);
  }
  if (type == PRODOS_T_SYS) {
    filetype = "SYSTEM";
  } else if (type == PRODOS_T_BIN) {
    filetype = "BIN";
  } else {
    filetype = "TXT";
  }

#else

  if (argc < 2) {
    printf("Usage: %s [file(s) to send]\n", argv[0]);
    exit(1);
  }

  if (simple_serial_open() < 0) {
    exit(1);
  }

send_again:

  if (get_filedetails(argv[cur_file], &filename, &filesize, &type, &auxtype) < 0) {
    printf("Can't get %s details\n", argv[cur_file]);
    exit(1);
  }

  fp = fopen(argv[cur_file],"r");
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
    filetype = "SYS";
  }
#endif

  /* Send filename */
#ifdef __CC65__
  simple_serial_printf("%s\n", remote_filename);
  printf("Filename sent:    %s\n", remote_filename);
#else
  simple_serial_printf("%s\n", filename);
  printf("Filename sent:    %s\n", filename);
#endif

  /* Send filetype */
  simple_serial_printf("%s\n", filetype);
  printf("Filetype sent:    %s\n", filetype);

#ifndef __CC65__
  if (!strcasecmp(filetype, "BIN")) {
    char buf[58];
    if (fread(buf, 1, 58, fp) == 58
     && buf[0] == 0x00 && buf[1] == 0x05
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
  simple_serial_printf("%lu\n", filesize - ftell(fp));

  printf("Data length sent: %lu\n", filesize - ftell(fp));

#ifndef __CC65__
  if (!strcasecmp(filetype, "BIN")) {
    simple_serial_printf("%02x%02x\n", start_addr[0], start_addr[1]);
    printf("Start address:    %02x%02x\n", start_addr[0], start_addr[1]);
  }
#endif

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
  free(path);
#else
  if (cur_file < argc -1) {
    cur_file++;
    count = 0;
    printf("\n");
    goto send_again;
  }
#endif
  simple_serial_close();
  printf("Wrote %d bytes.\n", count);
  exit(0);
}
