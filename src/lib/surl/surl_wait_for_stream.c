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

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#endif


int surl_wait_for_stream(void) {
  int r;
  int x, y;
  x = wherex();
  y = wherey();
  /* Cheap progress bar */
  cputs("\177\177\177\177\177\177\177\177\177\177");
  gotoxy(x,y);
  revers(1);
  while (1) {
    r = simple_serial_getc_with_timeout();
    switch (r) {
      case SURL_ANSWER_STREAM_LOAD:
        cputc(' ');
        if (kbhit()) {
          if (cgetc() == CH_ESC) {
            simple_serial_putc(SURL_METHOD_ABORT);
            simple_serial_getc(); /* Get the ERROR */
            r = -1;
            goto out;
          }
        }
        simple_serial_putc(SURL_CLIENT_READY);
        break;
      case SURL_ANSWER_STREAM_ERROR:
        r = -1;
        goto out;
      case SURL_ANSWER_STREAM_READY:
        r = 0;
        goto out;
    }
  }
out:
  revers(0);
  return r;
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif

#ifdef SURL_TO_LANGCARD
#pragma code-name (pop)
#endif
