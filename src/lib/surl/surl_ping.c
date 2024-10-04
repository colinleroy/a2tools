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
  const surl_response *resp;

try_again:
  clrscr();
  cputs("Establishing serial connection... ");
  resp = surl_start_request(NULL, 0, "ping://", SURL_METHOD_PING);
  if (resp->code != SURL_PROTOCOL_VERSION) {
    if (resp->code == 404) {
      simple_serial_flush();
      goto try_again;
    }
    cputs("\r\n\r\n");
    switch(resp->code) {
      case 504: 
        cputs("Timeout");
        break;
      case 600:
        cputs("Can not open serial port");
        break;
      default:
        if (resp->code != SURL_PROTOCOL_VERSION) {
          cputs("surl-server Protocol ");
          citoa(SURL_PROTOCOL_VERSION);
          cputs(" required");
        } else {
          cputs("Unknown error ");
          citoa(resp->code);
        }
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
