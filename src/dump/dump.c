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
#include "extended_conio.h"
#include <string.h>
#include <dbg.h>
#include <errno.h>
#include "platform.h"

#define BUF_SIZE 255
#define DATA_SIZE 16385

int main(void) {
  int l = 0, exit_code = 0;
  char *filename = malloc(BUF_SIZE);
  char *buf = malloc(DATA_SIZE);
  size_t b = 0;
  FILE *infp = NULL;

  if (buf == NULL) {
    printf("Couldn't allocate data buffer.\n");
    exit (1);
  }

  printf("Enter filename: ");
  cgets(filename, BUF_SIZE);

  _filetype = PRODOS_T_TXT;

  infp = fopen(filename,"r");
  if (infp == NULL) {
    printf("Open error %d: %s\n", errno, strerror(errno));
    exit_code = 1;
    goto err_out;
  }
  
  while (fgets(buf, DATA_SIZE, infp) != NULL) {
    int line_len;
    printf("%s", buf);
    line_len = strlen(buf);
    b += line_len;

    l+= 1 + line_len/40;

    if (l >= 23) {
      printf("(Hit a key to continue)\n");
      while(!kbhit());
      cgetc();
      gotoxy(0,22);
      l = 0;
    }
  }

  if (feof(infp)) {
    printf("\n(End of file. %d bytes read)\n", b);
  } else if (ferror(infp)) {
    printf("\nError %d: %s\n", errno, strerror(errno));
    
  }
  
  if (fclose(infp) != 0) {
    printf("Close error %d: %s\n", errno, strerror(errno));
  }

err_out:
  free(filename);
  free(buf);
  exit(0);
}
