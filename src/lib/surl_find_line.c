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
#ifndef __CC65__
#include <arpa/inet.h>
#endif
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

int __fastcall__ surl_find_line(char *buffer, size_t max_len, char *search_str) {
  size_t res_len = 0;
  char r;

  res_len = htons(max_len);
  simple_serial_putc(SURL_CMD_FIND);
  simple_serial_write((char *)&res_len, 2);
  simple_serial_puts(search_str);
  simple_serial_putc('\n');

  r = simple_serial_getc();
  if (r == SURL_ERROR_NOT_FOUND) {
    buffer[0] = '\0';
    return -1;
  }

  simple_serial_read((char *)&res_len, 2);

  /* coverity[tainted_return_value] */
  res_len = ntohs(res_len);

  simple_serial_read(buffer, res_len);

  buffer[res_len] = '\0';

  return 0;
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SURL_TO_LANGCARD
#pragma code-name (pop)
#endif
