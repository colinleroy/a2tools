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
#include "strellipsis.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#endif

static char *do_ellipsis(char *str, int len) {
  str[len-3] =
    str[len-2] =
    str[len-1] = '.';
  str[len]='\0';

  return str;
}

char *strndup_ellipsis(char *in, int len) {
  char *out;
  if (strlen(in) < len) {
    return strdup(in);
  }
  out = malloc(len + 1);
  strncpy(out, in, len - 1);
  return do_ellipsis(out, len);
}

char *ellipsis(char *in, int len) {
  if (strlen(in) < len) {
    return in;
  }
  return do_ellipsis(in, len);
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif
