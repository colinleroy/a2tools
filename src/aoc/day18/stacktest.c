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
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif

#define BUFSIZE 255
static void read_file(FILE *fp);

#define DATASET "IN16E"

int stack_size_test(void *ptr, int x, int y, int z, int d) {
  char result = 0;
  printf("%d.", d);
  return result || stack_size_test(ptr, x, y, z, d + 1);
}

int main(void) {
  char *buf;
  int i, j;
  char ***cubes = NULL;
  int num_cubes = 0;
  int surface_area = 0;
  int exterior_surface_area = 0;
  char *s_x, *s_y, *s_z;
  int x, y, z;
  
  cubes = malloc(16000);
  return stack_size_test(cubes, 1, 2, 3, 0);
}
