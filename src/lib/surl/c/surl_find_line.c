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

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#endif

static unsigned char in_body = 1;
static int __fastcall__ surl_find_in(char *buffer, char *search_str, size_t max_len, MatchType matchtype) {
  size_t res_len = 0;
  char r;

  simple_serial_putc(in_body ? SURL_CMD_FIND:SURL_CMD_FIND_HEADER);
  simple_serial_putc(matchtype);
  res_len = htons(max_len);
  simple_serial_write((char *)&res_len, 2);
  simple_serial_puts_nl(search_str);

  r = simple_serial_getc();
  if (r != SURL_ERROR_OK) {
    buffer[0] = '\0';
    return -1;
  }

  surl_read_with_barrier((char *)&res_len, 2);

  /* coverity[tainted_return_value] */
  res_len = ntohs(res_len);

  surl_read_with_barrier(buffer, res_len);

  buffer[res_len] = '\0';

  return 0;
}

int __fastcall__ surl_find_line(char *buffer, char *search_str, size_t max_len, MatchType matchtype) {
  in_body = 1;
  return surl_find_in(buffer, search_str, max_len, matchtype);
}

int __fastcall__ surl_find_header(char *buffer, char *search_str, size_t max_len, MatchType matchtype) {
  in_body = 0;
  return surl_find_in(buffer, search_str, max_len, matchtype);
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SURL_TO_LANGCARD
#pragma code-name (pop)
#endif
