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
 * along with this program. If not, see <surl://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "math.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#endif

#ifdef SERIAL_TO_LANGCARD
#pragma code-name (push, "LC")
#endif

#define BUFSIZE 255

int __fastcall__ surl_get_json(surl_response *resp, char *buffer, size_t max_len, char striphtml, char *translit, char *selector) {
  size_t res_len = 0;
  simple_serial_printf("JSON %zu %d %s %s\n", max_len, striphtml, translit ? translit : "0", selector);
  simple_serial_gets(buffer, max_len);

  if (!strcmp(buffer, "<NOT_FOUND>\n") || !strcmp(buffer, "<NOT_JSON>\n")) {
    buffer[0] = '\0';
    return -1;
  }
  res_len = atoi(buffer);
  simple_serial_read(buffer, sizeof(char), res_len);

  if (res_len > 0)
    buffer[res_len] = '\0';
  else
    buffer[0] = '\0';

  return 0;
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SERIAL_TO_LANGCARD
#pragma code-name (pop)
#endif
