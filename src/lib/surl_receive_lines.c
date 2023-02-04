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

static char overwritten_char = '\0';
static size_t overwritten_offset = 0;

size_t __fastcall__ surl_receive_lines(surl_response *resp, char *buffer, size_t max_len) {
  size_t to_read = min(resp->size - resp->cur_pos, max_len);
  size_t r = 0;
  size_t net_len;
  size_t last_return = 0;
  char *w;
  /* If we had cut the buffer short, restore the overwritten character,
   * move the remaining of the buffer to the start, and read what size
   * we have left */

  if (overwritten_char != '\0') {
    *(buffer + overwritten_offset) = overwritten_char;
    memmove(buffer, buffer + overwritten_offset, max_len - overwritten_offset);
    r = max_len - overwritten_offset;
    overwritten_char = '\0';
    to_read -= r;
  }

  if (to_read == 0) {
    return 0;
  }

  w = buffer + r;

  net_len = htons(to_read);
  simple_serial_putc(SURL_CMD_SEND);
  simple_serial_write((char *)&net_len, 1, 2);

  while (to_read > 0) {
    *w = simple_serial_getc();

    if(*w == '\n') {
      last_return = r;
    }

    ++w;
    ++r;
    --to_read;
  }
  
  /* Change the character after the last \n in the buffer
   * to a NULL byte, so the caller gets a full line,
   * and remember it. We'll reuse it at next read.
   */
  if (last_return > 0 && last_return + 1 < max_len) {
    overwritten_offset = last_return + 1;
    overwritten_char = *(buffer + overwritten_offset);
    r = overwritten_offset;
  }

  buffer[r] = '\0';
  resp->cur_pos += r;

  if (resp->cur_pos == resp->size) {
    overwritten_char = '\0';
  }

  return r;
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SERIAL_TO_LANGCARD
#pragma code-name (pop)
#endif
