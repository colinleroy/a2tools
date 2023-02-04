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

#ifdef SERIAL_TO_LANGCARD
#pragma code-name (push, "LC")
#endif

#define BUFSIZE 255

int __fastcall__ surl_get_json(surl_response *resp, char *buffer, size_t len, char striphtml, char *translit, char *selector) {
  char r;
  len = htons(len);

  simple_serial_putc(SURL_CMD_JSON);
  simple_serial_write((char *)&len, 1, 2);
  simple_serial_putc(striphtml);
  simple_serial_puts(translit ? translit : "0");
  simple_serial_putc(' ');
  simple_serial_puts(selector);
  simple_serial_putc('\n');

  r = simple_serial_getc();

  if (r == SURL_ERROR_NOT_FOUND || r == SURL_ERROR_NOT_JSON) {
    buffer[0] = '\0';
    return -1;
  }

  simple_serial_read((char *)&len, 1, 2);
  len = ntohs(len);

  simple_serial_read(buffer, sizeof(char), len);

  buffer[len] = '\0';

  return len;
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SERIAL_TO_LANGCARD
#pragma code-name (pop)
#endif