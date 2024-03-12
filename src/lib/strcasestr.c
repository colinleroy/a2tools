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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

char *strcasestr(char *str, char *substr) {
  int i = 0;
  int len_a = strlen(str);
  int len_b = strlen(substr);

  if (len_b > len_a)
    return NULL;

  for (i = 0; i <= len_a - len_b; i++, str++) {
    if (!strncasecmp(str, substr, len_b))
      return str;
  }
  return NULL;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
