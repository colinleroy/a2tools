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
#include "extended_conio.h"
#include "extended_string.h"

#ifdef __CC65__
#pragma static-locals(push, on)

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#else
#pragma code-name (push, "LOWCODE")
#endif
#endif

char *simple_serial_buf = NULL;
void simple_serial_printf(const char* format, ...) {
  va_list args;

  if (simple_serial_buf == NULL)
    simple_serial_buf = malloc(SIMPLE_SERIAL_BUF_SIZE);

  va_start(args, format);
  vsnprintf(simple_serial_buf, SIMPLE_SERIAL_BUF_SIZE - 1, format, args);
  va_end(args);

  simple_serial_puts(simple_serial_buf);
}

#ifdef __CC65__
#pragma code-name (pop)
#pragma static-locals(pop)
#endif
