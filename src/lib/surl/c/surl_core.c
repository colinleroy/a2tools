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
#include <stdarg.h>
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "math.h"

#ifdef __CC65__
  #pragma static-locals(push, on)

  #ifdef SURL_TO_LANGCARD
  #pragma code-name (push, "LC")
  #else
  #pragma code-name (push, "LOWCODE")
  #endif
#endif

char *surl_user_agent = NULL;

static char proxy_opened = 0;
char surl_connect_proxy(void) {
  static char r;
  if (proxy_opened) {
    return 0;
  }
  r = simple_serial_open();
  //DEBUG("connected proxy: %d\n", r);
  proxy_opened = (r == 0);

  if (r == 0) {
    simple_serial_setup_no_irq_regs();
    /* Break previous session if needed */
    #ifdef __CC65__
    serial_putc_direct(SURL_METHOD_ABORT);
    simple_serial_set_irq(1);
    #endif
    simple_serial_flush();
    simple_serial_putc(SURL_METHOD_ABORT);
    simple_serial_putc('\n');
    simple_serial_flush();
  }
  return r;
}

void surl_disconnect_proxy(void) {
  simple_serial_close();
  proxy_opened = 0;
}

surl_response resp;

const surl_response * __fastcall__ surl_start_request(char **headers, unsigned char n_headers, char *url, const SurlMethod method) {
  int i;

  if (resp.content_type) {
    free(resp.content_type);
    resp.content_type = NULL;
  }

  bzero(&resp, sizeof(surl_response));

  if (proxy_opened == 0) {
    if (surl_connect_proxy() != 0) {
      resp.code = 600;
      return &resp;
    }
  }

  simple_serial_putc(method);
  simple_serial_puts_nl(url);

  while (n_headers > 0) {
    n_headers--;
    simple_serial_puts_nl(headers[n_headers]);
  }

  if (surl_user_agent) {
    simple_serial_puts("User-Agent: ");
    simple_serial_puts_nl(surl_user_agent);
  }

  simple_serial_putc('\n');

  i = simple_serial_getc_with_timeout();

  if (i == EOF) {
    resp.code = 504;
    return &resp;
  } else {
    switch (method) {
      case SURL_METHOD_RAW:
        if (i == SURL_ANSWER_RAW_START){
          goto ret_100;
        }
        break;

      case SURL_METHOD_GET:
      case SURL_METHOD_DELETE:
        if (i == SURL_ANSWER_WAIT) {
          surl_read_response_header();
          return &resp;
        }
        break;

      case SURL_METHOD_POST_DATA:
      case SURL_METHOD_POST:
      case SURL_METHOD_PUT:
        if (i == SURL_ANSWER_SEND_SIZE || i == SURL_ANSWER_SEND_NUM_FIELDS) {
          goto ret_100;
        }
        break;

      case SURL_METHOD_PING:
        if (i == SURL_ANSWER_PONG) {
          simple_serial_putc(SURL_CLIENT_READY);
          resp.code = simple_serial_getc();
          return &resp;
        }
        break;

      case SURL_METHOD_STREAM_VIDEO:
      case SURL_METHOD_STREAM_AUDIO:
      case SURL_METHOD_STREAM_AV:
        if (i == SURL_ANSWER_WAIT) {
          goto ret_100;
        }
        break;

      case SURL_METHOD_GETTIME:
        if (i == SURL_ANSWER_TIME) {
          resp.code = 200;
          return &resp;
        }
        break;

#ifndef __CC65__
      case SURL_METHOD_DEBUG:
      case SURL_METHOD_DUMP:
      case SURL_METHOD_VSDRIVE:
      case SURL_METHOD_ABORT:
        /* We're not supposed to surl_start_request with these methods,
         * rather just:
         * simple_serial_putc(SURL_METHOD_DEBUG);
         * simple_serial_printf("%s:%d\n", file, line);
         * etc
         */
         break;
#endif
    }
  }

  resp.code = 508;
  return &resp;

ret_100:
  resp.code = 100;
  return &resp;
}

void __fastcall__ surl_read_response_header(void) {
  if (resp.content_type) {
    free(resp.content_type);
  }

  surl_read_with_barrier((char *)&resp, 10);

/* coverity[var_assign] */
  resp.size = ntohl(resp.size);
/* coverity[var_assign] */
  resp.code = ntohs(resp.code);
/* coverity[var_assign] */
  resp.header_size = ntohs(resp.header_size);
/* coverity[var_assign] */
  resp.content_type_size = ntohs(resp.content_type_size);
  /* Includes the zero byte */

  resp.content_type = malloc0(resp.content_type_size);

  surl_read_with_barrier(resp.content_type, resp.content_type_size);
}

char __fastcall__ surl_response_ok(void) {
  return resp.code >= 200 && resp.code < 300;
}


int __fastcall__ surl_response_code(void) {
  return resp.code;
}

const char * __fastcall__ surl_content_type(void) {
  return resp.content_type;
}

#ifndef __CC65__
void __fastcall__ surl_read_with_barrier(char *buffer, size_t nmemb) {
  simple_serial_putc(SURL_CLIENT_READY);
  simple_serial_read(buffer, nmemb);
}
#endif

#ifdef __CC65__
#pragma code-name (pop)
#endif

#ifdef SER_DEBUG
extern char *simple_serial_buf;
void surl_do_debug(const char *file, int line, const char *format, ...) {
  unsigned short n_len, dbg_len;
  va_list args;

  simple_serial_putc(SURL_METHOD_DEBUG);
  simple_serial_printf("%s:%d\n", file, line);

  va_start(args, format);
  /* simple_serial_buf has been allocated by previous simple_serial_printf. */
  vsnprintf(simple_serial_buf, SIMPLE_SERIAL_BUF_SIZE, format, args);
  va_end(args);

  dbg_len = strlen(simple_serial_buf) + 1;
  n_len = htons(dbg_len);
  simple_serial_write((char *)&n_len, 2);
  simple_serial_write(simple_serial_buf, dbg_len);
}
#endif

#ifdef __CC65__
#pragma static-locals(pop)
#endif
