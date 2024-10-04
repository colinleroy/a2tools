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

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#else
#define __fastcall__
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "malloc0.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#pragma register-vars(push, on)
#endif

int __fastcall__ _strnsplit_int(char in_place, char *in, char split, char **tokens, size_t max_tokens) {
  size_t n_tokens;
  /* copy to avoid stack access */
  char *sep, *start;

  sep = start = in;

  n_tokens = 0;

  while (1) {
    sep = strchr(sep, split);
    if (sep) {
      *sep = '\0';
    }

    if (in_place)
      tokens[n_tokens] = start;
    else
      tokens[n_tokens] = strdup(start);

    if (++n_tokens == max_tokens) {
      break;
    }
    if (sep) {
      start = ++sep;
      /* Ignore empty last lines */
      if (*start == '\0') {
        break;
      }
    } else {
      break;
    }
  }

  return n_tokens;
}

int __fastcall__ _strsplit_int(char in_place, char *in, char split, char ***out) {
  size_t n_tokens;
  /* copy to avoid stack access */
  register char *src = in;

  if (!in) {
    *out = NULL;
    return 0;
  }

  n_tokens = 1;
  while (*src) {
    if (*src == split) {
      ++n_tokens;
    }
    ++src;
  }
  *out = malloc(n_tokens * sizeof(char *));
  if (*out == NULL) {
    return 0;
  }
  return _strnsplit_int(in_place, in, split, *out, n_tokens);
}

#ifdef __CC65__
#pragma register-vars(pop)
#pragma static-locals(pop)
#pragma code-name (pop)
#endif
