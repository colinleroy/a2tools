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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "extended_conio.h"
#include "md5.h"

#define BUFSIZE 255
static char buf[BUFSIZE];

int main(int argc, char **argv) {
  md5_ctxt *md5 = NULL;
  uint8_t *md5_res = NULL;
  char c;
  FILE *fp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

again:
  printf("Enter filename: ");
  cgets(buf, BUFSIZE);
  *strchr(buf, '\n') = '\0';

  fp = fopen(buf,"rb");
  if (fp == NULL) {
    printf("Error: %s\n", strerror(errno));
    goto again;
  }

  md5 = md5_init();
  while(fread(&c, 1, 1, fp) > 0) {
    md5_loop(md5, &c, 1);
  }

  md5_pad(md5);
  
  md5_res = md5_result(md5);
  for (c = 0; c < 16; c++) {
    printf("%02x", md5_res[c]);
  }
  printf("\n");
  free(md5_res);
  free(md5);
  fclose(fp);
  fp = NULL;

  goto again;
  
  exit(0);
}
