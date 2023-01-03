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

int strsplit(char *in, char split, char ***out) {
  int i;
  int n_tokens = 1;
  char **result;


  for (i = 0; in[i] != '\0'; i++) {
    if (in[i] == split) {
      n_tokens++;
    }
  }

  result = malloc(n_tokens * sizeof(void *));
  n_tokens = 0;
  for (i = 0; in[i] != '\0'; i++) {
    if (in[i] == split) {
      in[i] = '\0';
      result[n_tokens] = strdup(in);
      in = in + i + 1; i = 0;
      n_tokens++;
      if (*in == '\0')
        break;
    }
  }
  /* last token */
  result[n_tokens] = strdup(in);
  n_tokens++;
  
  *out = result;
  return n_tokens;
}

char *trim(const char *in) {
  int i = 0, o = 0, len = strlen(in);
  int trimmed_start = 0, last_non_sep = 0;
  char *out = strdup(in);
  
  for (i = 0; i < len; i++) {
    int is_whitespc = strchr(" \r\n\t", in[i]) != NULL;
    if (!trimmed_start && is_whitespc)
      continue;
    trimmed_start = 1;
    if (trimmed_start) {
      if (last_non_sep == 0 && is_whitespc)
        last_non_sep = o;
      else if (last_non_sep > 0 && !is_whitespc)
        last_non_sep = 0;
    }

    out[o] = in[i];
    o++;
  }
  if (last_non_sep > 0) {
    out[last_non_sep]='\0';
  }

  out[o] = '\0';
  return out;
}
