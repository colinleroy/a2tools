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

int __fastcall__ surl_send_data_params(uint32 total, int mode) {
  total = htonl(total);
  mode = htons(mode);
  simple_serial_write((char *)&total, 4);
  simple_serial_write((char *)&mode, 2);

  /* Wait for go */
  return (simple_serial_getc() == SURL_UPLOAD_GO) ? 0 : -1;
}

void __fastcall__ surl_send_data(char *data, size_t chunk_size) {
  size_t net_size = htons(chunk_size);
  simple_serial_putc(SURL_CLIENT_READY);
  simple_serial_write((char *)&net_size, 2);
  simple_serial_write(data, chunk_size);
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SURL_TO_LANGCARD
#pragma code-name (pop)
#endif
