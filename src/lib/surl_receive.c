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
#include <arpa/inet.h>
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

#define BUFSIZE 255

extern surl_response *resp;

size_t __fastcall__ surl_receive_data(char *buffer, size_t max_len) {
  size_t to_read = min(resp->size - resp->cur_pos, max_len);
  size_t r;

  if (to_read == 0) {
    return 0;
  }

  r = htons(to_read);
  simple_serial_putc(SURL_CMD_SEND);
  simple_serial_write((char *)&r, 2);
  simple_serial_read(buffer, to_read);

  buffer[to_read] = '\0';
  resp->cur_pos += to_read;

  return to_read;
}

void surl_strip_html(char strip_level) {
  simple_serial_putc(SURL_CMD_STRIPHTML);
  simple_serial_putc(strip_level);
  surl_read_response_header();
}

void surl_translit(char *charset) {
  simple_serial_putc(SURL_CMD_TRANSLIT);
  simple_serial_puts(charset);
  simple_serial_putc('\n');
  surl_read_response_header();
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SURL_TO_LANGCARD
#pragma code-name (pop)
#endif
