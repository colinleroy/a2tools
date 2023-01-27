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
#include "extended_string.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#endif

static int __fastcall__ _strsplit_int(char in_place, char *in, char split, char ***out) {
  int i;
  int n_tokens;
  char **result = NULL;
  char **tmp;

  n_tokens = 0;
  if (in == NULL)
    goto err_out;

  for (i = 0; in[i] != '\0'; i++) {
    if (in[i] == split) {
      tmp = realloc(result, (n_tokens + 1) * sizeof(void *));
      if (tmp) {
        result = tmp;
      } else {
        free(result);
        n_tokens = 0;
        goto err_out;
      }
      in[i] = '\0';
      result[n_tokens] = in_place ? in : strdup(in);
      in = in + i + 1; i = -1; /* going to be ++'ed by the for loop */
      n_tokens++;
      if (*in == '\0')
        break;
    }
  }
  /* last token */
  if (in && *in) {
    tmp = realloc(result, (n_tokens + 1) * sizeof(void *));
    if (tmp) {
      result = tmp;
    } else {
      free(result);
      n_tokens = 0;
      goto err_out;
    }
    result[n_tokens] = in_place ? in : strdup(in);
    n_tokens++;
  }

err_out:  
  *out = result;
  return n_tokens;
}

int __fastcall__ strsplit(char *in, char split, char ***out) {
  return _strsplit_int(0, in, split, out);
}

int __fastcall__ strsplit_in_place(char *in, char split, char ***out) {
  return _strsplit_int(1, in, split, out);
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif
