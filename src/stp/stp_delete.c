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
#include "extended_conio.h"
#include "stp_delete.h"
#include "stp.h"
#include "stp_cli.h"
#include "get_buf_size.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "math.h"

#define APPLESINGLE_HEADER_LEN 58

extern unsigned char scrw, scrh;

void stp_delete_dialog(char *url, char *filename) {
  char c;

  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 7);

  cprintf("Delete %s?", filename);

  do {
    c = cgetc();
  } while (c != CH_ENTER && c != CH_ESC);

  if (c == CH_ENTER) {
    gotoxy(0, 17);
    cprintf("Deleting file...            ");
    stp_delete(url, filename);
  }
}

void stp_delete(char *url, char *filename) {
  char *full_url;
  const surl_response *resp = NULL;

  if (scrw == 255)
    screensize(&scrw, &scrh);

  full_url = malloc(strlen(url) + 1 + strlen(filename) + 1);
  sprintf(full_url, "%s/%s", url, filename);

  resp = surl_start_request(SURL_METHOD_DELETE, full_url, NULL, 0);

  free(full_url);

  stp_print_result(resp);

  gotoxy(0, 17);
  cprintf("Done. hit a key to continue.      ");
  cgetc();
}
