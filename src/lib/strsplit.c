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
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "extended_string.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#endif

static int __fastcall__ _strsplit_int(char in_place, char *in, char split, char ***out, size_t max_tokens) {
  char *start;
  size_t n_tokens;
  char **tokens;
  /* copy to avoid stack access */
  char *src = in;

  if (!in) {
    *out = NULL;
    return 0;
  }

  if (max_tokens == 0) {
    start = src;
    n_tokens = 1;
    while (*src) {
      if (*src == split) {
        ++n_tokens;
      }
      ++src;
    }

    tokens = malloc(n_tokens * sizeof(char *));
    src = start;
  } else {
    tokens = malloc((max_tokens) * sizeof(char *));
    start = src;
  }

  n_tokens = 0;

  while (*src) {
    if (*src == split) {
      *src = '\0';

      if (in_place)
        tokens[n_tokens] = start;
      else
        tokens[n_tokens] = strdup(start);

      ++n_tokens;
      start = src + 1;

      /* no need to check where max_tokens != 0,
       * n_tokens won't be 0 by that point */
      if (n_tokens == max_tokens) {
        goto done;
      }
    }
    ++src;
  }

  /* Last token */
  if (*start) {
      if (in_place)
        tokens[n_tokens] = start;
      else
        tokens[n_tokens] = strdup(start);
    ++n_tokens;
  }

done:
  *out = tokens;
  return n_tokens;
}

int __fastcall__ strsplit(char *in, char split, char ***out) {
  return _strsplit_int(0, in, split, out, 0);
}

int __fastcall__ strsplit_in_place(char *in, char split, char ***out) {
  return _strsplit_int(1, in, split, out, 0);
}

int __fastcall__ strnsplit(char *in, char split, char ***out, char max_tokens) {
  return _strsplit_int(0, in, split, out, max_tokens);
}

int __fastcall__ strnsplit_in_place(char *in, char split, char ***out, char max_tokens) {
  return _strsplit_int(1, in, split, out, max_tokens);
}

#ifdef __CC65__
#pragma static-locals(pop)
#pragma code-name (pop)
#endif
