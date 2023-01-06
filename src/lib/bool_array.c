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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bool_array.h"

struct _bool_array {
  int x_len;
  int y_len;
  char *data;
};

static size_t alloc_size(int xlen, int ylen) {
  size_t allocsize = 1 + (xlen / 8);

  return allocsize * ylen;
}

bool_array *bool_array_alloc(int xlen, int ylen) {
    size_t allocsize = alloc_size(xlen, ylen);
    bool_array *array = malloc(sizeof(struct _bool_array));
    char *data = NULL;

    data = malloc(allocsize);

    if (data == NULL) {
      free(array);
      return NULL;
    }
    memset(data, 0, allocsize);

    array->x_len = xlen;
    array->y_len = ylen;
    array->data = data;

    return array;
}

void bool_array_free(bool_array *array) {
  if (array != NULL) {
    if (array->data != NULL) {
      free(array->data);
    }
    free(array);
  }
}

#define BOOL_ARRAY_SET_BIT(a,x,y) (a->data[x] |= (1 << (y)))
#define BOOL_ARRAY_CLR_BIT(a,x,y) (a->data[x] &= ~(1 << (y)))
#define BOOL_ARRAY_GET_BIT(a,x,y) (a->data[x] & (1 << (y)))

void __fastcall__ bool_array_set(bool_array *array, int x, int y, int val) {
  long ba_offset = (long)((long)(array->y_len) * (long)x) + (long)y;

  if (val == 0)
    BOOL_ARRAY_CLR_BIT(array, ba_offset >> 3, ba_offset & 7);
  else
    BOOL_ARRAY_SET_BIT(array, ba_offset >> 3, ba_offset & 7);
}

int __fastcall__ bool_array_get(bool_array *array, int x, int y) {
  long ba_offset = (long)((long)(array->y_len) * (long)x) + (long)y;

  return BOOL_ARRAY_GET_BIT(array, ba_offset >> 3, ba_offset & 7);
}
