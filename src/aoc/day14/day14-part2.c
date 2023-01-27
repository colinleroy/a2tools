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
#include <conio.h>
#include <apple2enh.h>
#include <tgi.h>
#endif
#include "slist.h"
#include "bool_array.h"
#include "extended_string.h"
#include "strsplit.h"

#define DATASET "IN14"
#define BUFSIZE 300

static bool_array *read_file(FILE *fp);
static void print_obstacles(bool_array *obstacles);
static int sand_fall(int num, bool_array *obstacles);

int main(void) {
  FILE *fp;
  bool_array *obstacles;
  int count = 0;
  char c;
#ifdef DO_TGI 
  unsigned char palette[2] = { TGI_COLOR_WHITE, TGI_COLOR_BLACK };
#endif

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(DATASET, "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }

  obstacles = read_file(fp);

  printf("Ready.\n");
#ifdef DO_TGI
  tgi_install(a2_hi_tgi);
  tgi_init ();
  tgi_setpalette(palette);
  tgi_setcolor(TGI_COLOR_BLACK);
  tgi_clear();
  tgi_setcolor(TGI_COLOR_WHITE);
#endif

  print_obstacles(obstacles);

  while (sand_fall(count, obstacles)) {
    count ++;
  }
  count++;

#ifdef DO_TGI
  tgi_done();
#endif

  printf("Count: %d\n", count);
  bool_array_free(obstacles);

  exit (0);
}

static int x_offset, y_offset;
static int map_w, map_h;
static int mid_screen_x;
static int mid_screen_y;
static int bottom_line;

#define OFF_X(x) (x - x_offset)
#define OFF_Y(y) (y - y_offset)

static bool_array *read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  char **coords, **numbers;
  int num_coords = 0, num_numbers = 0, i, t;
  int start_x, start_y, end_x, end_y;
  int next_start_x = 0, next_start_y = 0, x, y;
  int min_x, max_x, min_y = 0, max_y;
  int num_lines = 0;
  bool_array *obstacles;

  printf("Calibrating map...");
  do {
    if (fgets(buf, BUFSIZE-1, fp) == NULL)
      break;

    num_lines++;
    num_coords = strsplit(buf, ' ', &coords);
    for (i = 0; i < num_coords; i++) {
      if (coords[i][0] != '-') {
        num_numbers = strsplit(coords[i],',',&numbers);

        x = atoi(numbers[0]);
        y = atoi(numbers[1]);

        if (num_lines == 1) {
          min_x = x;
          max_x = x;
          /* not min_y. min_y is 0 */
          max_y = y;
        }
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y > max_y) max_y = y;

        free (numbers[0]);
        free (numbers[1]);
        free(numbers);
      }
      free(coords[i]);
    }
    free(coords);
    printf(".");
  } while (1);
  printf("\n");

  bottom_line = max_y + 2;
  min_x = 500 - bottom_line;
  max_x = 500 + bottom_line;
  
  x_offset = min_x;
  y_offset = min_y;
  map_w = max_x - min_x;
  map_h = bottom_line - min_y;

  mid_screen_x = (240 - map_w) / 2;
  mid_screen_y = (190 - map_h) / 2;

  printf("Map coords: (%d,%d) to (%d,%d).\n", min_x, min_y, max_x, max_y);
  printf("Floor: %d.\n", bottom_line);
  printf("Shifted map by (%d,%d)\n", x_offset, y_offset);
  printf("Map is now (%d,%d) to (%d,%d)\n", OFF_X(min_x), OFF_Y(min_y),
          OFF_X(max_x), OFF_Y(max_y));

  obstacles = bool_array_alloc(map_w + 1, map_h + 1);

  rewind(fp);

  printf("Setting up segments...");
  do {
    if (fgets(buf, BUFSIZE-1, fp) == NULL)
      break;

    num_coords = strsplit(buf, ' ', &coords);

    i = 0;

    num_numbers = strsplit(coords[i],',',&numbers);
    next_start_x = atoi(numbers[0]);
    next_start_y = atoi(numbers[1]);

    i+= 2;

    free(numbers[0]);
    free(numbers[1]);
    free(numbers);

    do {
      start_x = next_start_x;
      start_y = next_start_y;

      num_numbers = strsplit(coords[i],',',&numbers);
      end_x = atoi(numbers[0]);
      end_y = atoi(numbers[1]);

      next_start_x = end_x;
      next_start_y = end_y;

      if (start_x > end_x) {
        t = end_x;
        end_x = start_x;
        start_x = t;
      }
      if (start_y > end_y) {
        t = end_y;
        end_y = start_y;
        start_y = t;
      }

      i+= 2;

      free(numbers[0]);
      free(numbers[1]);
      free(numbers);

      for (x = start_x; x <= end_x; x++) {
        for (y = start_y; y <= end_y; y++) {
          bool_array_set(obstacles, OFF_X(x), OFF_Y(y), 1);
        }
      }

    } while (i < num_coords - 0);

    while (num_coords > 0)
      free(coords[--num_coords]);
    free(coords);

    printf(".");
  } while (!feof(fp));
  printf("\n");

  free(buf);
  fclose(fp);

  return obstacles;
}

static void print_obstacles(bool_array *obstacles) {
  int x, y;

  for (y = 0; y <= map_h; y++) {
    for (x = 0; x <= map_w; x++) {
#ifdef DO_TGI
      if (bool_array_get(obstacles, x, y) == 1) {
        tgi_setpixel(x + mid_screen_x, y + mid_screen_y);
      }
#else
      printf("%c", (bool_array_get(obstacles, x, y) == 1) ? '#':' ');
#endif
    }
#ifdef DO_TGI
    tgi_line(0, OFF_Y(bottom_line) + mid_screen_y, 
             240, OFF_Y(bottom_line) + mid_screen_y);
#else
    printf(" %d\n", y);
#endif
  }
}

static void update_sand(int x, int y) {
#ifdef DO_TGI
  if (x > 0 && y > 0) {
    tgi_setpixel(OFF_X(x), OFF_Y(y));
  }
#endif
}

static int sand_fall(int num, bool_array *obstacles) {
  int x = 500, y = 0;

  do {
    if (y < bottom_line - 1 && bool_array_get(obstacles, OFF_X(x), OFF_Y(y + 1)) == 0) {
      y++;
    } else {
      if (y < bottom_line - 1 && !bool_array_get(obstacles, OFF_X(x - 1), OFF_Y(y + 1))) {
        x--;
        y++;
      } else if (y < bottom_line - 1 && !bool_array_get(obstacles, OFF_X(x + 1), OFF_Y(y + 1))) {
        x++;
        y++;
      } else if (x == 500 && y == 0) {
#ifndef DO_TGI
          printf("sand %d can't fall anymore (%d,%d)\n", num, x, y);
#endif
          return 0;
        } else {
        update_sand(x, y);
        bool_array_set(obstacles, OFF_X(x), OFF_Y(y), 1);
#ifndef DO_TGI
        if (num % 20 == 0) {
          printf("grain of sand %d stopped at (%d,%d)\n\n\n", num, x, y);
        }
#endif
        return 1;
      }
    }
  } while(1);
}
