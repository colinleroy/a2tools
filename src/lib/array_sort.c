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
#include "array_sort.h"

void bubble_sort_array(void **array, size_t n_elts, sort_func func) {
  int i;
  int swapped_anything;
  if (n_elts < 1) {
    return;
  }

again:
  swapped_anything = 0;
  for (i = 0; i < n_elts - 1; i++) {
    if (func(array[i], array[i + 1]) > 0) {
      void *tmp = array[i];
      array[i] = array[i + 1];
      array[i + 1] = tmp;
      swapped_anything = 1;
    }
  }
  if (swapped_anything)
    goto again;
}
