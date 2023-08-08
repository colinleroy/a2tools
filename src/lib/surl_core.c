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

#ifdef SERIAL_TO_LANGCARD
#pragma code-name (push, "LC")
#else
#pragma code-name (push, "LOWCODE")
#endif
#endif

#define BUFSIZE 255

static char proxy_opened = 0;
int surl_connect_proxy(void) {
  int r;
#ifdef __CC65__
  r = simple_serial_open(2, SER_BAUD_9600);
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

surl_response * __fastcall__ surl_start_request(const char method, char *url, char **headers, int n_headers) {
  surl_response *resp;
  int i;

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

  simple_serial_putc(method);
  simple_serial_puts(url);
  simple_serial_putc('\n');
  //DEBUG("sent req %s %s\n", method, url);
  for (i = 0; i < n_headers; i++) {
    simple_serial_puts(headers[i]);
    simple_serial_putc('\n');
    //DEBUG("sent hdr %d %s\n", i, headers[i]);
  }
  simple_serial_putc('\n');

  i = simple_serial_getc_with_timeout();

  if (i == EOF) {
    resp->code = 504;
    return resp;
  } else if (method == SURL_METHOD_GET && i != SURL_ANSWER_WAIT) {
    resp->code = 508;
    return resp;
  } else if (method == SURL_METHOD_DELETE && i != SURL_ANSWER_WAIT) {
    resp->code = 508;
    return resp;
  } else if (method == SURL_METHOD_RAW && i == SURL_ANSWER_RAW_START) {
    resp->code = 100;
    return resp;
  } else if (method == SURL_METHOD_GETTIME && i == SURL_ANSWER_TIME) {
    resp->code = 200;
    return resp;
  } else if (i == SURL_ANSWER_SEND_SIZE || i == SURL_ANSWER_SEND_NUM_FIELDS) {
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

  simple_serial_read((char *)resp, 6);
  simple_serial_gets(surl_buf, BUFSIZE);

/* coverity[var_assign] */
  resp->code = ntohs(resp->code);
/* coverity[var_assign] */
  resp->size = ntohs(resp->size);
/* coverity[var_assign] */
  resp->header_size = ntohs(resp->header_size);
  resp->content_type = strdup(surl_buf);

  w = strchr(resp->content_type, '\n');
  if (w)
    *w = '\0';
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
#pragma code-name (pop)
#endif
