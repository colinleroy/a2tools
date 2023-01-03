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
#include <conio.h>
#endif
#include "bool_array.h"
#include "extended_conio.h"
#include "math.h"

#define BUFSIZE 10100

#define NUM_ITERATIONS 2022
#define WALLS_WIDTH 7

static void read_file(FILE *fp);

#define DATASET "IN17E"

int shape_masks[5][4][4] = {
  {{0,0,0,0},
   {0,0,0,0},
   {0,0,0,0},
   {1,1,1,1}
 },
  {{0,0,0,0},
   {0,1,0,0},
   {1,1,1,0},
   {0,1,0,0}
 },
  {{0,0,0,0},
   {0,0,1,0},
   {0,0,1,0},
   {1,1,1,0}
 },
  {{1,0,0,0},
   {1,0,0,0},
   {1,0,0,0},
   {1,0,0,0}
 },
  {{0,0,0,0},
   {0,0,0,0},
   {1,1,0,0},
   {1,1,0,0}
 },
};

int shape_widths[5]  = {4, 3, 3, 1, 2};
int shape_heights[5] = {1, 3, 3, 4, 2};

int main(void) {
  FILE *fp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(DATASET, "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }

  read_file(fp);

  exit (0);
}

#define WIND_RIGHT 1
#define WIND_LEFT 0

static char *blank_line = NULL;
static char *cutoff_line = NULL;
static char *bottom_line = NULL;

static void setup_viz(void) {
  clrscr();
  cvlinexy(0, 0, 23);
  chlinexy(1, 22, 7);
  cvlinexy(8, 0, 23);
  gotoxy(WALLS_WIDTH + 10, 12);
  puts("Iterations        ");
  gotoxy(WALLS_WIDTH + 10, 13);
  puts("    Height        ");
  gotoxy(WALLS_WIDTH + 10, 15);
  puts("      Wind        ");
}

#define VIZ
#ifdef VIZ
static int going_fast = 0;
static int viz_is_setup = 0;
static int round_fh = 0;
static void viz(char *msg, int iterations, bool_array *table, int cur_shape, int cur_shape_left, int cur_shape_bottom, int cur_wind, int floor_height) {
  int cur_shape_top = cur_shape_bottom + 4;
  int max_fh = floor_height;
  int line = max_fh + 10;
  int x, y, s_x, s_y;
  int zone_x_s, zone_y_s, zone_x_e, zone_y_e;
  int debug = 0;
#ifdef __CC65__
  gotoxy(1, 1);
  if (kbhit()) {
    cgetc();
    going_fast = !going_fast;
    clrscr();
    viz_is_setup = 0;
  }
  if (going_fast) {
    gotoxy(2, 10);
    puts("Computing really fast behind the scene\n");
    gotoxy(6, 12);
    printf("Iterations: %d, Height: %d       \n", iterations, floor_height);
    return;
  }
#endif
  // if (iterations != NUM_ITERATIONS) {
  //   if (iterations > 10 && iterations % 9)
  //     return;
  //   else if (iterations > 100 && iterations % 99)
  //     return;
  // }

#ifndef __CC65__
  printf("\n");
  for (y = max(round_fh, 19); y >= round_fh - 15 && y >= 0; y--) {
    printf("I");
    for (x = 0; x < WALLS_WIDTH; x++) {
      if (y <= cur_shape_top - 1 && y > cur_shape_bottom - 1
       && x >= cur_shape_left && x < cur_shape_left + shape_widths[cur_shape]
       && shape_masks[cur_shape][cur_shape_top - y - 1][x - cur_shape_left]) {
        printf("o");
      } else if (bool_array_get(table, x, y)) {
        printf("+");
      } else {
        printf(" ");
      }
    }
    printf("I (%d)\n", y);
  }
  if (y > 1) {
    printf("|%s|\n", cutoff_line);
  }
  printf("+%s+ (0) - %s (%d,%d)\n", bottom_line, msg, iterations, floor_height);
#else
  if (!strcmp(msg,"start round")) {
    if(!viz_is_setup) {
      setup_viz();
      viz_is_setup = 1;
    }
    round_fh = cur_shape_top;

    zone_y_s = max(round_fh, 22); s_y = 0;
    zone_y_e = max(round_fh, 22) - 21;

    //clrzone(1,0,7,22); /* Useless as I'll rewrite it all */
    zone_x_s = 0; s_x = 1;
    zone_x_e = WALLS_WIDTH;

    gotoxy(WALLS_WIDTH +2, 0);
    printf("%d    ", zone_y_s);
    gotoxy(WALLS_WIDTH +2, 22);
    printf("%d    ", zone_y_e - 1);

    /* 12 = strlen("Iterations") + 2 */
    gotoxy(WALLS_WIDTH + 10 + 12, 12);
    printf("%d  ", iterations);
    gotoxy(WALLS_WIDTH + 10 + 12, 13);
    printf("%d  ", floor_height);
    debug = 0;
  } else {
    int d_y;
    if(!viz_is_setup) {
      /* we need a round start to determine offsets */
      return;
    }

    zone_y_s = max(round_fh, 22); s_y = 0;
    zone_y_e = max(round_fh, 22) - 21;

    /* Narrow to current shape + 1 border w/h */
    zone_x_s = max(cur_shape_left - 1, 0);    s_x = zone_x_s + 1;
    zone_x_e = min(cur_shape_left + shape_widths[cur_shape] + 1, WALLS_WIDTH);

    d_y       = zone_y_s - (cur_shape_bottom + shape_heights[cur_shape]);
    zone_y_s  = cur_shape_bottom + shape_heights[cur_shape];
    s_y      += d_y;

    zone_y_e = max(zone_y_s - shape_heights[cur_shape] - 1, 1);
  }
  for (y = zone_y_s; y >= zone_y_e && y >= 0; y--, s_y++) {
    gotoxy(zone_x_s + 1, s_y);
    for (x = zone_x_s; x < zone_x_e; x++, s_x++) {
      if (y <= cur_shape_top - 1 && y > cur_shape_bottom - 1
       && x >= cur_shape_left && x < cur_shape_left + shape_widths[cur_shape]
       && shape_masks[cur_shape][cur_shape_top - y - 1][x - cur_shape_left]) {
        cputc('o');
      } else if (bool_array_get(table, x, y)) {
        cputc('+');
      } else {
        cputc(' ');
      }
    }
  }
  gotoxy(WALLS_WIDTH + 10 + 12, 15);
  printf("%c", cur_wind == WIND_LEFT ? '<':'>');
#endif
}
#else
#define viz(msg, iterations, table, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height) do {} while (0)
#endif

static int can_move_left(bool_array *table, int shape, int x, int y) {
  int col, row, d_x, d_y;

  if (x == 0)
    return 0;

  /* check each column from down to top */
  for (row = 3; row >= 0; row --) {
    d_y = y + 3 - row;
    for (col = 0; col < 4; col ++) {
      d_x = x + col;
      if (shape_masks[shape][row][col] && bool_array_get(table, d_x - 1, d_y))
        return 0;
    }
  }
  return 1;
}

static int can_move_right(bool_array *table, int shape,int x, int y) {
  int col, row, d_x, d_y;

  if (x + shape_widths[shape] == 7)
    return 0;

  /* check each column from down to top */
  for (row = 3; row >= 0; row --) {
    d_y = y + 3 - row;
    for (col = 0; col < 4; col ++) {
      d_x = x + col;
      if (shape_masks[shape][row][col] && bool_array_get(table, d_x + 1, d_y))
        return 0;
    }
  }
  return 1;
}

static int can_move_down(bool_array *table, int shape, int x, int y) {
  int col, row, d_x, d_y;

  if (y == 1)
    return 0;

  /* check each column from down to top */
  for (row = 3; row >= 0; row --) {
    d_y = y + 3 - row;
    for (col = 0; col < 4; col ++) {
      d_x = x + col;
      if (shape_masks[shape][row][col] && bool_array_get(table, d_x, d_y - 1))
        return 0;
    }
  }
  return 1;
}

static void land(bool_array *table, int shape, int x, int y, int *floor_height) {
  int s_x, s_y, t_x, t_y;

  for (s_x = 0, t_x = x; s_x < shape_widths[shape]; s_x++, t_x++) {
    for (s_y = 3, t_y = y; s_y >= 0; s_y--, t_y++) {
      if (shape_masks[shape][s_y][s_x]) {
        bool_array_set(table, t_x, t_y, 1);
        if (t_y > *floor_height) {
          *floor_height = t_y;
        }
      }
    }
  }
}

static void simulate_falls(bool_array *wind, bool_array *table, int wind_pattern_length) {
  int cur_shape = 0;
  int iterations = 0;
  int floor_height = 0;
  int wind_count = 0;
  char cur_wind;
  int cur_shape_bottom, cur_shape_left;
  int must_land;

#ifdef __CC65__
  clrscr();
#endif

  do {
    cur_shape_bottom = 4 + floor_height;
    cur_shape_left = 2;
    must_land = 0;
    viz("start round", 
          iterations, table, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height);

    while (1) {
      cur_wind = (bool_array_get(wind, wind_count, 0)) == 0 ? WIND_LEFT:WIND_RIGHT;
      /* get pushed */
      if (cur_wind == WIND_RIGHT && can_move_right(table, cur_shape, cur_shape_left, cur_shape_bottom)) {
        cur_shape_left++;
      } else if (cur_wind == WIND_LEFT && can_move_left(table, cur_shape, cur_shape_left, cur_shape_bottom))
        cur_shape_left--;

      viz(cur_wind == WIND_LEFT ? "left" : "right", 
            iterations, table, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height);

      /* change wind for next time */
      wind_count = (wind_count + 1) % wind_pattern_length;

      /* get down */
      if (can_move_down(table, cur_shape, cur_shape_left, cur_shape_bottom)) {
        cur_shape_bottom--;
      } else {
        land(table, cur_shape, cur_shape_left, cur_shape_bottom, &floor_height);
        must_land = 0;
        break;
      }
        

      viz("down", 
            iterations, table, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height);
    }

    iterations++;
    cur_shape = (cur_shape + 1) % 5;
  } while (iterations <= NUM_ITERATIONS);
  
  viz("done!", 
        iterations, table, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height);

}

static void read_file(FILE *fp) {
  bool_array *wind, *table;
  char c;
  int i = 0;
  int wind_pattern_length = 0;

  wind = bool_array_alloc(12000, 1);

  while ((c = fgetc(fp)) != '\n' && c != '\0') {
    bool_array_set(wind, i++, 0, (c == '>'));
    wind_pattern_length++;
  }

  fclose(fp);

  table = bool_array_alloc(7, 5000);

  blank_line = malloc((1 + WALLS_WIDTH) * sizeof(char));
  cutoff_line = malloc((1 + WALLS_WIDTH) * sizeof(char));
  bottom_line = malloc((1 + WALLS_WIDTH) * sizeof(char));
  for (i = 0; i < WALLS_WIDTH; i++) {
    blank_line[i] = ' ';
    cutoff_line[i] = '.';
    bottom_line[i] = '-';
    bool_array_set(table, i, 0, 1);
  }
  blank_line[i] = '\0';
  cutoff_line[i] = '\0';
  bottom_line[i] = '\0';

  simulate_falls(wind, table, wind_pattern_length);
  bool_array_free(wind);
}
