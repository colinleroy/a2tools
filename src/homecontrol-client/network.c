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

#include <stdio.h>
#include <stdlib.h>
#include "network.h"
#include "simple_serial.h"

char *get_url(char *url) {
  char *buffer = NULL;
  extern surl_response resp;

  surl_start_request(NULL, 0, url, SURL_METHOD_GET);

  if (surl_response_code() == 200) {
    buffer = malloc(resp.size + 1);
    if (buffer != NULL) {
      surl_receive_data(buffer, resp.size);
    }
  }
  return buffer;
}
