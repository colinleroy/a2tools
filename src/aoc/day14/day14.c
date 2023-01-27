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
#include <tgi.h>
#else
#include "tgi_compat.h"
#endif
#include "extended_conio.h"
#include "bool_array.h"
#include "extended_string.h"
#include "strsplit.h"

/* Init HGR segment */
#pragma rodata-name (push, "HGR")
const char hgr = 0;
#pragma rodata-name (pop)

#define DATASET "IN14"
#define BUFSIZE 300

static int part = 2;
static bool_array *read_file(FILE *fp);

static int sand_fall(int num, bool_array *obstacles);
static int simulate_sand(bool_array *obstacles);

int main(void) {
  FILE *fp;
  bool_array *obstacles;
  int count = 0;

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

  if (part == 1) {
    while (sand_fall(count, obstacles)) {
      count ++;
    }
  } else {

    tgi_install(a2e_hi_tgi);
    tgi_init ();
    tgi_setcolor(TGI_COLOR_WHITE);

    count = simulate_sand(obstacles);

    tgi_done();
  }

  printf("Count: %d\n", count);
  bool_array_free(obstacles);

  exit (0);
}

static int x_offset, y_offset;
static int map_w, map_h;
static int mid_screen_x;
static int mid_screen_y;

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

  x_offset = min_x;
  y_offset = min_y;
  map_w = max_x - min_x;
  map_h = max_y - min_y;

  mid_screen_x = ((260 - map_w) / 2) + 1;
  mid_screen_y = ((190 - map_h) / 2);

  printf("Map coords: (%d,%d) to (%d,%d).\n", min_x, min_y, max_x, max_y);
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

static int sand_fall(int num, bool_array *obstacles) {
  int x = 500, y = 0;
  int prev_x = x;
  int prev_y = y;

  do {
    if (!bool_array_get(obstacles, OFF_X(x), OFF_Y(y + 1))
           && OFF_Y(y) <= map_h) {
      y++;
    } else {
      if (OFF_X(x) > 0 && !bool_array_get(obstacles, OFF_X(x - 1), OFF_Y(y + 1))) {
        x--;
        y++;
      } else if (OFF_X(x) < map_w && !bool_array_get(obstacles, OFF_X(x + 1), OFF_Y(y + 1))) {
        x++;
        y++;
      } else if (OFF_X(x) <= 0) {
        x--;
        y++;
      } else if (OFF_X(x) >= map_w) {
        x++;
        y++;
      } else {
        printf("grain of sand %d stopped at (%d,%d)\n", num, x, y);
        bool_array_set(obstacles, OFF_X(x), OFF_Y(y), 1);
        return 1;
      }
      if (OFF_X(x) < 0 || OFF_X(x) >= map_w || OFF_Y(y) > map_h) {
#ifndef __CC65__
        printf("grain of sand %d fell out at (%d,%d)\n", num, x, y);
#endif
        return 0;
      }
      prev_x = x;
      prev_y = y;
    }
  } while(1);
}

#define LINE_ARRAY_OFF(a) (a-(500-(map_h+2)))
static int simulate_sand(bool_array *obstacles) {
  int max_line_w = ((map_h + 2) * 2) + 1;
  bool_array *line, *prev_line;
  int line_min_x, line_max_x, x, y;
  int count = 0;
  int ba_x, ba_y, l_x;

  prev_line = bool_array_alloc(max_line_w, 1);
  
  /* top line */
  bool_array_set(prev_line, LINE_ARRAY_OFF(500), 0, 1);
  line_min_x = 500;
  line_max_x = 500;

  /* go down to bottom */
  for (y = 0; y <= map_h + 1; y++) {
    line = bool_array_alloc(max_line_w, 1);

#if 0
    for (int i = 0; i <= LINE_ARRAY_OFF(line_min_x); i++) {
      printf(" ");
    }
#endif

    for (x = line_min_x; x <= line_max_x; x++) {
      ba_x = OFF_X(x);
      ba_y = OFF_Y(y);
      l_x = LINE_ARRAY_OFF(x);

      /* is this an obstacle ? */
      if (ba_x >= 0 && ba_x <= map_w && bool_array_get(obstacles, ba_x, ba_y)) {
#if 0
        printf("#");
#else
        tgi_setcolor(TGI_COLOR_WHITE);
        tgi_setpixel(ba_x + mid_screen_x, ba_y + mid_screen_y);
#endif
        continue;
      }
      /* was there a grain of sand ? */
      if (bool_array_get(prev_line, l_x, 0) == 0) {
#if 0
        printf(" ");
#endif
        continue;
      } else {
          count++;
#if 0
        printf("o");
#else
        tgi_setcolor(TGI_COLOR_ORANGE);
        if (ba_x + mid_screen_x >= 0 && ba_x + mid_screen_x < 280) {
          tgi_setpixel(ba_x + mid_screen_x, ba_y + mid_screen_y);
        }
#endif
      }

      if (y < map_h+1) {
        /* is there an obstacle at the left ? */
        if (bool_array_get(line, l_x - 1, 0) == 0 && (ba_x - 1 < 0 || ba_x - 1 > map_w || !bool_array_get(obstacles, ba_x-1, ba_y+1))) {
          
          bool_array_set(line, l_x - 1, 0, 1);
        } 
        /* is there an obstacle just under ? */
        if (bool_array_get(line, l_x, 0) == 0 && (ba_x < 0 || ba_x > map_w || !bool_array_get(obstacles, ba_x, ba_y+1))) {
          bool_array_set(line, l_x, 0, 1);
        } 
        /* is there an obstacle at the right ? */
        if (ba_x +1 < 0 || ba_x +1 > map_w || !bool_array_get(obstacles, ba_x + 1, ba_y + 1)) {
          bool_array_set(line, l_x + 1, 0, 1);
        }
      }
    }
#if 0
    printf(" %d\n", y);
#endif
    bool_array_free(prev_line);
    prev_line = line;
    line = NULL;
    line_min_x--;
    line_max_x++;
  }
  bool_array_free(prev_line);
  return count;
}
