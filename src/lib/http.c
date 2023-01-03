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
#include "http.h"
#include "simple_serial.h"
#include "extended_conio.h"

#define BUFSIZE 255

#ifndef __CC65__
#include "http_curl.c"

#else
static char proxy_opened = 0;
void http_connect_proxy(void) {
#ifdef __CC65__
  simple_serial_open(2, SER_BAUD_9600);
  simple_serial_set_timeout(10);
#endif
  proxy_opened = 1;
}

static char buf[BUFSIZE];

http_response *http_request(const char *method, const char *url, const char **headers, int n_headers) {
  http_response *resp;
  int i;

  if (proxy_opened == 0) {
    http_connect_proxy();
  }

  resp = malloc(sizeof(http_response));
  if (resp == NULL) {
    return NULL;
  }

  resp->body = NULL;
  resp->size = 0;
  resp->code = 0;

  simple_serial_printf("%s %s\n", method, url);

  for (i = 0; i < n_headers; i++) {
    simple_serial_printf("%s\n", headers[i]);
  }
  simple_serial_puts("\n");

  simple_serial_gets_with_timeout(buf, BUFSIZE);
  if (buf == NULL || buf[0] == '\0') {
    resp->code = 509;
    return resp;
  }

  if (strchr(buf, ',') == NULL) {
    resp->code = 510;
    return resp;
  }

  resp->size = atol(strchr(buf, ',') + 1);
  *strchr(buf,',') = '\0';
  resp->code = atoi(buf);

  resp->body = malloc(resp->size + 1);
  if (resp->body == NULL) {
    resp->code = 511;
    return resp;
  }

  simple_serial_read(resp->body, sizeof(char), resp->size + 1);
  resp->body[resp->size] = '\0';

  return resp;
}

void http_response_free(http_response *resp) {
  free(resp->body);
  free(resp);
}
#endif
