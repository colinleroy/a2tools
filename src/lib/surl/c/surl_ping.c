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
#include <ctype.h>
#include <arpa/inet.h>
#ifndef __CC65__
#include "extended_conio.h"
#else
#include <conio.h>
#endif
#include "surl.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "citoa.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#pragma code-name (push, "RT_ONCE")
#endif

void __fastcall__ surl_ping(void) {
  int response_code;

try_again:
  clrscr();
  cputs("Establishing serial connection... ");
  surl_start_request(NULL, 0, "ping://", SURL_METHOD_PING);
  response_code = surl_response_code();
  if (response_code != SURL_PROTOCOL_VERSION) {
    if (response_code == SURL_ERR_PROTOCOL_ERROR) {
      simple_serial_flush();
      goto try_again;
    }
    cputs("\r\n\r\n");
    switch(response_code) {
      case SURL_ERR_TIMEOUT:
        cputs("Timeout");
        break;
      case SURL_ERR_PROXY_NOT_CONNECTED:
        cputs("Can not open serial port");
        break;
      default:
        cputs("surl-server Protocol ");
        citoa(SURL_PROTOCOL_VERSION);
        cputs(" required, got ");
        citoa(response_code);
    }
    cputs(". Press any key to try again or C to configure.\r\n");

    if (tolower(cgetc()) == 'c') {
      surl_disconnect_proxy();
      simple_serial_configure();
    }
    goto try_again;
  }

  clrscr();
}

#ifdef __CC65__
#pragma code-name(pop)
#pragma static-locals(pop)
#endif
