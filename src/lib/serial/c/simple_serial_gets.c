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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "simple_serial.h"

#ifdef __CC65__
  #pragma static-locals(push, on)
  #pragma code-name (push, "LOWCODE")
  extern unsigned char open_slot;
#else
#include <sys/stat.h>
  extern int ttyfd;
  extern char *opt_tty_path;
#endif

#ifdef __CC65__
#error Function too slow to use as is
#endif

char * __fastcall__ simple_serial_gets(char *out, size_t size) {
  static char c;
  static char *cur;
  static char *end;
  static int last_err = 0;
  if (size == 0) {
    return NULL;
  }

  cur = out;
  end = cur + size - 1;
  while (cur < end) {
#ifdef __CC65__
    while (ser_get(&c) == SER_ERR_NO_DATA);
#else
    c = simple_serial_getc();
    if (c == (char)EOF) {
      switch (errno) {
        case EBADF:
        case EIO:
          return NULL;
        default:
          if (errno != last_err) {
            printf("Read error %d (%s)\n", errno, strerror(errno));
            last_err = errno;
          }
      }
    }
#endif
    
    if (c == '\r') {
      /* ignore \r */
      continue;
    }

    *cur = c;
    ++cur;

    if (c == '\n') {
      break;
    }
  }
  *cur = '\0';

  return out;
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif
