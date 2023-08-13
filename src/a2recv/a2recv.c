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
#include <stdlib.h>
#include "simple_serial.h"
#include <string.h>
#include <errno.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "clrzone.h"

#define BUF_SIZE 255
#define DATA_SIZE 16384
#define FLOPPY_DELAY 300000

int main(void) {
  int w, exit_code = 0;
  int floppy_delay;
  char *filename = malloc(BUF_SIZE);
  char *filetype = malloc(BUF_SIZE);
  char *s_len = malloc(BUF_SIZE);
#ifdef __CC65__
  char *start_addr = malloc(BUF_SIZE);
#endif
  size_t data_len = 0;
  FILE *outfp = NULL;
  char *data = NULL;
  unsigned int total;


#ifdef __CC65__
  if (simple_serial_open(2, SER_BAUD_9600) < 0) {
    exit(1);
  }
#else
  if (simple_serial_open() < 0) {
    exit(1);
  }
#endif

read_again:
  printf("\nReady to receive (Ctrl-reset to abort)\n");
  total = 0;

read_filename_again:
  if (simple_serial_gets(filename, BUF_SIZE) != NULL) {
    if (!strcmp(filename,"\4\n")) {
      goto read_filename_again;
    }
#ifndef __CC65__
    if (strlen(filename) > 8)
      filename[8] = '\0';
#endif
    if (strchr(filename, '\n'))
      *strchr(filename, '\n') = '\0';
    printf("Filename   '%s'\n", filename);
  }
  if (simple_serial_gets(filetype, BUF_SIZE) != NULL) {
    if (strchr(filetype, '\n'))
      *strchr(filetype, '\n') = '\0';
    printf("Filetype   '%s'\n", filetype);
  }

  if (simple_serial_gets(s_len, BUF_SIZE) != NULL) {
    if (strchr(s_len, '\n'))
      *strchr(s_len, '\n') = '\0';
    data_len = atoi(s_len);
    printf("Data length %u\n", (unsigned int)data_len);
  }

#ifdef __CC65__
  if (!strcasecmp(filetype, "TXT")) {
    _filetype = PRODOS_T_TXT;
    _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  } else if (!strcasecmp(filetype,"BIN")) {
    _filetype = PRODOS_T_BIN;
    simple_serial_gets(start_addr, BUF_SIZE);
    _auxtype = strtoul(start_addr, NULL, 16);
    printf("Start address %04x\n", _auxtype);
  } else if (!strcasecmp(filetype,"SYS")) {
    char *tmp = malloc(BUF_SIZE);
    sprintf(tmp, "%s.system", filename);
    free(filename);
    filename = tmp;
    _filetype = PRODOS_T_SYS;
  } else {
    printf("Filetype is unsupported.\n");
    exit_code = 1;
    goto err_out;
  }
#else
  char *tmp = malloc(BUF_SIZE);
  sprintf(tmp, "%s.%s", filename, filetype);
  free(filename);
  filename = tmp;
#endif


  while (data_len > 0) {
    size_t block = (data_len > DATA_SIZE ? DATA_SIZE : data_len);
    data = malloc(block + 1);

    if (data == NULL) {
      printf("Couldn't allocate %d bytes of data.\n", (int)block);
      exit (1);
    }

    simple_serial_set_activity_indicator(1, -1, -1);

    printf("Reading data...");
    fflush(stdout);
    simple_serial_read(data, block);
    total += block;

    simple_serial_set_activity_indicator(0, -1, -1);

    printf("\nRead %u bytes. Writing %s...\n", total, filename);


    if (outfp == NULL)
      outfp = fopen(filename,"w");
    if (outfp == NULL) {
      printf("Open error %d: %s\n", errno, strerror(errno));
      exit_code = 1;
      goto err_out;
    }

    w = fwrite(data, 1, block, outfp);
    if (w < block) {
      printf("Only wrote %d bytes: %s\n", w, strerror(errno));
      exit_code = 1;
      goto err_out;
    }

    free(data);
    data_len -= block;
    if (data_len > 0) {
      for (floppy_delay = FLOPPY_DELAY; floppy_delay > 0; floppy_delay--) {
        /* Wait for floppy to be done */
      }
      printf("Telling sender we're ready\n");
      simple_serial_puts("READY\n");
    }
  }

  if (fclose(outfp) != 0) {
    printf("Close error %d: %s\n", errno, strerror(errno));
    exit_code = 1;
  }

  outfp = NULL;

  printf("Telling sender we're done\n");
  simple_serial_puts("READY\n");
  
  goto read_again;

err_out:
  free(filename);
  free(filetype);
  free(data);

  simple_serial_close();
  exit(exit_code);
}
