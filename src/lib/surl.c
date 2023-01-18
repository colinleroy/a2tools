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

static char proxy_opened = 0;
int surl_connect_proxy(void) {
  int r;
#ifdef __CC65__
  r = simple_serial_open(2, SER_BAUD_9600, 1);
#else
  r = simple_serial_open();
#endif
  //DEBUG("connected proxy: %d\n", r);
  proxy_opened = (r == 0);
  return r;
}

void surl_close_proxy(void) {
  simple_serial_close();
}

static char buf[BUFSIZE];

surl_response *surl_start_request(const char *method, const char *url, const char **headers, int n_headers) {
  surl_response *resp;
  int i;
  char got_buf;
  if (proxy_opened == 0) {
    if (surl_connect_proxy() != 0) {
      return NULL;
    }
  }

  resp = malloc(sizeof(surl_response));
  if (resp == NULL) {
    //DEBUG("cant alloc resp\n");
    return NULL;
  }

  resp->size = 0;
  resp->code = 0;
  resp->cur_pos = 0;
  resp->content_type = NULL;

  simple_serial_printf("%s %s\n", method, url);
  //DEBUG("sent req %s %s\n", method, url);
  for (i = 0; i < n_headers; i++) {
    simple_serial_printf("%s\n", headers[i]);
    //DEBUG("sent hdr %d %s\n", i, headers[i]);
  }
  simple_serial_puts("\n");

  got_buf = simple_serial_gets_with_timeout(buf, BUFSIZE) != NULL;
  printf("buf '%s'", buf);
  if (!got_buf || *buf == '\0') {
    resp->code = 504;
    return resp;
  } else if (!strcmp(method, "GET") && strcmp(buf, "WAIT\n")) {
    resp->code = 508;
    return resp;
  } else if (!strcmp(method, "RAW") && !strcmp(buf, "RAW_SESSION_START\n")) {
    resp->code = 100;
    return resp;
  } else if (!strcmp(buf, "SEND_SIZE_AND_DATA\n")) {
    resp->code = 100;
    return resp;
  }

  surl_read_response_header(resp);
  return resp;
}

void surl_send_data_size(surl_response *resp, size_t total) {
  simple_serial_printf("%zu\n", total);
}

size_t surl_send_data(surl_response *resp, char *buffer, size_t len) {
  return simple_serial_write(buffer, 1, len);
}

void surl_read_response_header(surl_response *resp) {
  char *w;

  simple_serial_gets(buf, BUFSIZE);
  //DEBUG("RESPonse header %s\n", buf);
  if (buf[0] == '\0') {
    resp->code = 509;
    return;
  }

  if (strchr(buf, ',') == NULL) {
    resp->code = 510;
    return;
  }

  resp->size = atol(strchr(buf, ',') + 1);
  w = strchr(buf,',') + 1;
  *strchr(buf,',') = '\0';
  resp->code = atoi(buf);

  if (strchr(w, ',') != NULL) {
    resp->content_type = strdup(strchr(w, ',') + 1);
    if (strchr(resp->content_type, '\n'))
      *strchr(resp->content_type, '\n') = '\0';
  } else {
    resp->content_type = strdup("application/octet-stream");
  }  
}

size_t surl_receive_data(surl_response *resp, char *buffer, size_t max_len) {
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

size_t surl_receive_lines(surl_response *resp, char *buffer, size_t max_len) {
  size_t to_read = min(resp->size - resp->cur_pos, max_len);
  size_t r = 0;
  size_t last_return = 0;

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

  simple_serial_printf("SEND %zu\n", to_read);

  while (to_read > 0) {
    *(buffer + r) = simple_serial_getc();

    if(*(buffer + r) == '\n') {
      last_return = r;
    }

    r ++;
    to_read--;
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

  if (buffer[r-1] != '\n') {
    exit(1);
  }
  buffer[r] = '\0';
  resp->cur_pos += r;

  if (resp->cur_pos == resp->size) {
    overwritten_char = '\0';
  }

  return r;
}

void surl_response_free(surl_response *resp) {
  if (resp == NULL) {
    return;
  }
  free(resp->content_type);
  free(resp);
  /* Flush serial */
  simple_serial_flush();

}
//Pop early because the whole serial + surl code doesn't fit in LC
#ifdef SERIAL_TO_LANGCARD
#pragma code-name (pop)
#endif

#ifdef __CC65__
#pragma static-locals(pop)
#endif
