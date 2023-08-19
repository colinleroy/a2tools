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
#include "extended_conio.h"
#else
#include <conio.h>
#endif
#include "surl.h"
#include "simple_serial.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#endif

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#endif

void __fastcall__ surl_ping(void) {
  const surl_response *resp;
  char done = 0;
try_again:
  clrscr();
  gotoxy(0, 10);
  cputs("Establishing serial connection... ");
  resp = surl_start_request(SURL_METHOD_PING, "ping://", NULL, 0);
  if (resp->code == SURL_PROTOCOL_VERSION) {
    done = 1;
  } else {
    cprintf("\r\n\r\n%s. Press any key to try again...\r\n",
            resp->code == 504 ? "Timeout"
              : resp->code != SURL_PROTOCOL_VERSION ? "Wrong protocol version"
                : "Unknown error");
    cgetc();
  }

  if (!done) {
    goto try_again;
  }
  cputs("Done.");
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SURL_TO_LANGCARD
#pragma code-name (pop)
#endif
