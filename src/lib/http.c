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
#include "math.h"

#define BUFSIZE 255

static char proxy_opened = 0;
void http_connect_proxy(void) {
#ifdef __CC65__
  simple_serial_open(2, SER_BAUD_9600, 1);
#else
  if (getenv("SERIAL_TTY") == NULL) {
    printf("Please export SERIAL_TTY=/dev/ttyUSB0 or something.\n");
    exit(1);
  }
  simple_serial_open(getenv("SERIAL_TTY"), B9600, 1);
#endif
  proxy_opened = 1;
}

void http_close_proxy(void) {
  simple_serial_close();
}

static char buf[BUFSIZE];

http_response *http_start_request(const char *method, const char *url, const char **headers, int n_headers) {
  http_response *resp;
  int i;

  if (proxy_opened == 0) {
    http_connect_proxy();
  }

  resp = malloc(sizeof(http_response));
  if (resp == NULL) {
    return NULL;
  }

  resp->size = 0;
  resp->code = 0;
  resp->cur_pos = 0;

  simple_serial_printf("%s %s\n", method, url);

  for (i = 0; i < n_headers; i++) {
    simple_serial_printf("%s\n", headers[i]);
  }
  simple_serial_puts("\n");

  simple_serial_gets_with_timeout(buf, BUFSIZE);
  if (buf == NULL || strcmp(buf, "WAIT\n")) {
    resp->code = 508;
    return resp;
  }

  simple_serial_gets(buf, BUFSIZE);
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

  return resp;
}

size_t http_receive_data(http_response *resp, char *buffer, size_t max_len) {
  size_t to_read = min(resp->size - resp->cur_pos, max_len);
  size_t r;

  if (to_read == 0) {
    return 0;
  }

  simple_serial_printf("SEND %zu\n", to_read);
  r = simple_serial_read(buffer, sizeof(char), to_read);

  buffer[r] = '\0';
  resp->cur_pos += r;

  return r;
}

static char overwritten_char = '\0';
static size_t overwritten_offset = 0;

size_t http_receive_lines(http_response *resp, char *buffer, size_t max_len) {
  size_t to_read = min(resp->size - resp->cur_pos, max_len);
  size_t r = 0;
  size_t last_return = 0;

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

  simple_serial_printf("SEND %zu\n", to_read);

  while (to_read > 0) {
    *(buffer + r) = simple_serial_getc();

    if(*(buffer + r) == '\n') {
      last_return = r;
    }

    r ++;
    to_read--;
  }
  if (last_return > 0 && last_return + 1 < max_len - 1) {
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

void http_response_free(http_response *resp) {
  free(resp);
}
