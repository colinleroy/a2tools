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

  if (r == 0) {
    /* Break previous session if needed */
    simple_serial_printf("%c\n", 0x04);
    simple_serial_flush();
  }
  return r;
}

char surl_buf[BUFSIZE];

surl_response * __fastcall__ surl_start_request(const char method, const char *url, char **headers, int n_headers) {
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
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }

  resp->size = 0;
  resp->code = 0;
  resp->cur_pos = 0;
  resp->content_type = NULL;
  resp->header_size = 0;
  resp->cur_hdr_pos = 0;

  simple_serial_printf("%c%s\n", method, url);
  //DEBUG("sent req %s %s\n", method, url);
  for (i = 0; i < n_headers; i++) {
    simple_serial_puts(headers[i]);
    simple_serial_putc('\n');
    //DEBUG("sent hdr %d %s\n", i, headers[i]);
  }
  simple_serial_putc('\n');

  got_buf = simple_serial_gets_with_timeout(surl_buf, BUFSIZE) != NULL;

  if (!got_buf || *surl_buf == '\0') {
    resp->code = 504;
    return resp;
  } else if (method == SURL_METHOD_GET && strcmp(surl_buf, "WAIT\n")) {
    resp->code = 508;
    return resp;
  } else if (method == SURL_METHOD_DELETE && strcmp(surl_buf, "WAIT\n")) {
    resp->code = 508;
    return resp;
  } else if (method == SURL_METHOD_RAW && !strcmp(surl_buf, "RAW_SESSION_START\n")) {
    resp->code = 100;
    return resp;
  } else if (!strcmp(surl_buf, "SEND_SIZE_AND_DATA\n")) {
    resp->code = 100;
    return resp;
  }

  surl_read_response_header(resp);
  return resp;
}

void __fastcall__ surl_read_response_header(surl_response *resp) {
  char *w;

  if (resp->content_type) {
    return; // already read.
  }

  simple_serial_gets(surl_buf, BUFSIZE);
  //DEBUG("RESPonse header %s\n", surl_buf);
  if (surl_buf[0] == '\0') {
    resp->code = 509;
    return;
  }

  if (strchr(surl_buf, ',') == NULL) {
    resp->code = 510;
    return;
  }

  w = surl_buf;
  resp->code = atoi(w);

  w = strchr(w, ',') + 1;
  resp->size = atol(w);

  w = strchr(w,',') + 1;
  resp->header_size = atol(w);

  if (strchr(w, ',') != NULL) {
    resp->content_type = strdup(strchr(w, ',') + 1);
    w = strchr(resp->content_type, '\n');
    if (w)
      *w = '\0';
  } else {
    resp->content_type = strdup("application/octet-stream");
  }  
}

size_t __fastcall__ surl_receive_data(surl_response *resp, char *buffer, size_t max_len) {
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

void __fastcall__ surl_response_free(surl_response *resp) {
  if (resp == NULL) {
    return;
  }
  free(resp->content_type);
  free(resp);
  /* Flush serial */
  simple_serial_flush();

}

char __fastcall__ surl_response_ok(surl_response *resp) {
  return resp != NULL && resp->code >= 200 && resp->code < 300;
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SERIAL_TO_LANGCARD
#pragma code-name (pop)
#endif
