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
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "stp_cli.h"
#include "extended_conio.h"

static unsigned char scrw = 255, scrh = 255;

void stp_print_header(char *url) {
  
  if (scrw == 255)
    screensize(&scrw, &scrh);

  clrzone(0, 0, scrw, 0);
  gotoxy(0,0);
  if (strlen(url) > scrw - 2 /* One char for serial act */) {
    char *tmp = strdup(url + strlen(url) - scrw + 5);
    printf("...%s", tmp);
    free(tmp);
  } else {
    printf("%s",url);
  }
  gotoxy(0, 1);
  chline(scrw);
}

void stp_print_result(surl_response *response) {
  if (scrw == 255)
    screensize(&scrw, &scrh);
  gotoxy(0, 18);
  chline(scrw);
  clrzone(0, 19, scrw, 20);
  gotoxy(0, 19);
  if (response == NULL) {
    printf("Unknown request error.");
  } else {
    printf("Response code %d - %zu bytes,\n%s", 
            response->code,
            response->size,
            response->content_type != NULL ? response->content_type : "");
  }

}

void stp_print_footer(void) {
  if (scrw == 255)
    screensize(&scrw, &scrh);
  gotoxy(0, 21);
  chline(scrw);
  clrzone(0, 22, scrw, 23);
  gotoxy(0, 22);
  printf("Up/Down: navigate\n"
       "Enter: select - Esc: back");
}
