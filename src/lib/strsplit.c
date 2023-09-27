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

int __fastcall__ _strnsplit_int(char in_place, char *in, char split, char **tokens, size_t max_tokens) {
  char *start;
  size_t n_tokens;
  /* copy to avoid stack access */
  char *src = in;

  start = src;

  n_tokens = 0;

  while (*src) {
    if (*src == split) {
      *src = '\0';

insert_token:
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
      /* make sure we break out of the loop */
      max_tokens = n_tokens + 1;
      goto insert_token;
  }

done:
  return n_tokens;
}

int __fastcall__ _strsplit_int(char in_place, char *in, char split, char ***out) {
  size_t n_tokens;
  /* copy to avoid stack access */
  char *src = in;

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
  return _strnsplit_int(in_place, in, split, *out, n_tokens);
}

#ifdef __CC65__
#pragma static-locals(pop)
#pragma code-name (pop)
#endif
