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
#include <unistd.h>
#ifndef __CC65__
#include "extended_conio.h"
#else
#include <conio.h>
#endif
#include "stp_delete.h"
#include "stp.h"
#include "stp_cli.h"
#include "get_buf_size.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "math.h"

#define APPLESINGLE_HEADER_LEN 58

extern char scrw, scrh;

void stp_delete_dialog(char *url, char *filename) {
  char c;

  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 7);
  printf("%s", filename);

  gotoxy(6, 10);
  printf("Delete file?");

  gotoxy(6, 16);
  chline(28);
  gotoxy(6, 17);
  printf("Esc: cancel !  Enter: Delete");
  do {
    c = cgetc();
  } while (c != CH_ENTER && c != CH_ESC);
  
  if (c == CH_ENTER) {
    gotoxy(6, 17);
    printf("Deleting file...            ");
    stp_delete(url, filename);
  }
}

void stp_delete(char *url, char *filename) {
  char *full_url;
  surl_response *resp = NULL;

  if (scrw == 255)
    screensize(&scrw, &scrh);

  full_url = malloc(strlen(url) + 1 + strlen(filename) + 1);
  sprintf(full_url, "%s/%s", url, filename);

  resp = surl_start_request(SURL_METHOD_DELETE, full_url, NULL, 0);

  free(full_url);

  stp_print_result(resp);
  surl_response_free(resp);

  gotoxy(6, 17);
  printf("Hit a key to continue.      ");
  cgetc();
}
