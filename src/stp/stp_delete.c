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
#include "stp_delete.h"
#include "stp_cli.h"
#include "get_buf_size.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "math.h"

#define APPLESINGLE_HEADER_LEN 58

static unsigned char scrw = 255, scrh = 255;

void stp_delete_dialog(char *url, char *filename) {
  char c;

  printxcenteredbox(30, 11);
  printxcentered(7, filename);

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
