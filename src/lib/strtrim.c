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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "strtrim.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#endif

char * __fastcall__ trim(const char *in) {
  int i = 0, len = strlen(in);
  int last_non_sep = 0;
  char *out;

  /* Front trim */
  while (i < len) {
    if (!strchr(" \r\n\t", in[i]))
      break;
    i++;
  }
  if (i == len)
    return strdup("");

  out = strdup(in + i);

  /* Tail trim */
  for (i = 0; out[i]; i++) {
    if (!strchr(" \r\n\t", out[i])) {
      last_non_sep = 0;
    } else if (last_non_sep == 0) {
      last_non_sep = i;
    }
  }

  if (last_non_sep != 0) {
    out[last_non_sep]='\0';
  }

  return out;
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif
