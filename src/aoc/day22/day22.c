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
#include "tgi_compat.h"
#include "extended_conio.h"
#include "bool_array.h"

/* Init HGR segment */
#pragma rodata-name (push, "HGR")
const char hgr = 0;
#pragma rodata-name (pop)

#define BUFSIZE 255
static void read_file(FILE *fp);

static void unit_tests_p1(void);
static void unit_tests_p2(void);

#define DATASET "IN22"

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

#define RIGHT 0
#define DOWN  1
#define LEFT  2
#define UP    3

/* 1 = walkable, 0 = obstacle */
static bool_array *tiles;
/* 1 = empty, 0 = is tile */
static bool_array *empty;

static int max_x = 0, max_y = 0;

static int my_direction = RIGHT;
static int my_x = -1, my_y = -1;

#define is_empty(x, y)    (bool_array_get(empty, (x), (y)) != 0)

#define is_obstacle(x, y) (bool_array_get(tiles, (x), (y)) == 0)

static void next_tile_p1(int *my_x, int *my_y, int *direction) {
  int dx, dy;
  int x = *my_x;
  int y = *my_y;

  switch (*direction) {
    case UP:    y = max_y - 1; dx =  0; dy = -1; break;
    case RIGHT: x = 0;         dx = +1; dy =  0; break;
    case DOWN:  y = 0;         dx =  0; dy = +1; break;
    case LEFT:  x = max_x - 1; dx = -1; dy =  0; break;
  }

  while (is_empty(x, y)) {
    /* skip over empty tiles */
    x += dx;
    y += dy;
  }

  /* is this an obstacle ? */
  if (is_obstacle(x, y)) {
    return;
  } else {
    *my_x = x; *my_y = y;
  }
}

static void next_tile_p2(int *my_x, int *my_y, int *my_direction) {
  int dx, dy;
  int x = *my_x;
  int y = *my_y;
  int direction = *my_direction;

  if (y == 0 && x >= 50 && x < 100 && direction == UP) {
    /* 2 to 6 */ direction = RIGHT; y = 150 + (x - 50); x = 0; 
  } else if (y == 0 && x >= 100 && x < 150 && direction == UP) {
    /* 1 to 6 */ direction = DOWN;    y = 199; x = x - 100;
  } else if (x == 50 && y >= 0 && y < 50 && direction == LEFT) {
    /* 2 to 5 */ direction = RIGHT; x = 0; y = 100 + (49 - y);
  } else if (y == 49 && x >= 100 && x < 150 && direction == DOWN) {
    /* 1 to 3 */ y = 50 + x - 100; x = 99; direction = LEFT;
  } else if (x == 149 && y >= 0 && y < 50 && direction == RIGHT) {
    /* 1 to 4 */ direction = LEFT; x = 99; y = 100 + (49 - y);
  } else if (x == 50 && y >= 50 && y < 100 && direction == LEFT) {
    /* 3 to 5 */ direction = DOWN; x = y - 50; y = 100;
  } else if (x == 99 && y >= 50 && y < 100 && direction == RIGHT) {
    /* 3 to 1 */ direction = UP; x = 100 + y - 50; y = 49;
  } else if (y == 100 && x >= 0 && x < 50 && direction == UP) {
    /* 5 to 3 */ direction = RIGHT; y = x + 50; x = 50;
  } else if (x == 0 && y >= 100 && y < 150 && direction == LEFT) {
    /* 5 to 2 */ direction = RIGHT; y = 49 - (y - 100); x = 50;
  } else if (x == 99 && y >= 100 && y < 150 && direction == RIGHT) {
    /* 4 to 1 */ direction = LEFT; y = 49 - (y - 100); x = 149;
  } else if (y == 149 && x >= 50 && x < 100 && direction == DOWN) {
    /* 4 to 6 */ direction = LEFT; y = 150 + x - 50; x = 49;
  } else if (x == 0 && y >= 150 && y < 200 && direction == LEFT) {
    /* 6 to 2 */ direction = DOWN; x = 50 + y - 150; y = 0;
  } else if (x == 49 && y >= 150 && y < 200 && direction == RIGHT) {
    /* 6 to 4 */ direction = UP;  x = 50 + y - 150; y = 149;
  } else if (y == 199 && x >= 0 && x < 50 && direction == DOWN) {
    /* 6 to 1 */ direction = UP; x = x + 100; y = 0;
  }
  
  /* is this an obstacle ? */
  if (is_obstacle(x, y)) {
    return;
  } else {
    *my_x = x; 
    *my_y = y;
    *my_direction = direction;
  }
}

static int x_offset, y_offset;

static void do_walk(int steps) {
  int i, dx, dy;
  switch (my_direction) {
    case UP:    dx =  0; dy = -1; break;
    case RIGHT: dx = +1; dy =  0; break;
    case DOWN:  dx =  0; dy = +1; break;
    case LEFT:  dx = -1; dy =  0; break;
  }
  for (i = 0; i < steps; i++) {
    if (my_x + dx < 0 || my_x + dx >= max_x
            || my_y + dy < 0 || my_y + dy >= max_y
            || is_empty(my_x + dx, my_y + dy)) {
      /* wrap around */
      int old_x = my_x, old_y = my_y;

      next_tile_p2(&my_x, &my_y, &my_direction);

      if (old_x == my_x && old_y == my_y) {
        return;
      }
      tgi_setcolor(TGI_COLOR_PURPLE);
      tgi_setpixel(my_x + x_offset, my_y + y_offset);
    } else if (is_obstacle(my_x + dx, my_y + dy) && !is_empty(my_x + dx, my_y + dy)) {
      /* stop at obstacle */
      return;
    } else {
      /* walk */
      my_x += dx;
      my_y += dy;
      tgi_setcolor(TGI_COLOR_PURPLE);
      tgi_setpixel(my_x + x_offset, my_y + y_offset);
    }
  }
}

static void do_turn(char d) {
  if (d == 'R') {
    my_direction = (my_direction + 1) % 4;
  } else {
    my_direction--;
    if (my_direction < RIGHT)
      my_direction = UP;
  }
}

static void test(int start_x, int start_y, int steps, int direction, int end_x, int end_y, char *label) {
  my_x = start_x;
  my_y = start_y;
  my_direction = direction;
  do_walk(steps);
  if (my_x != end_x || my_y != end_y) {
    printf("%s FAILED (from %d, %d expected %d,%d, got %d,%d)\n", label, start_x, start_y, end_x, end_y, my_x, my_y);
  }
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  int x = 0, y = 0, start_x = -1, start_y;
  char c;
  int i, walk;
  long result;

  /* get map size */
  printf("Sizing map...\n");
  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    int len = strlen(buf);

    *strchr(buf, '\n') = 0;
    len--;

    if (len == 0) {
      break;
    } else {
      if (len > max_x) {
        max_x = len;
      }
      max_y++;
    }
  };
  
  rewind(fp);
  printf("Map is %d*%d\n", max_x, max_y);
  /* fill in map */
  tiles = bool_array_alloc(max_x, max_y);
  empty = bool_array_alloc(max_x, max_y);

  x_offset = (280 - max_x) / 2;
  y_offset = (200 - max_y) / 2;

  tgi_install(a2e_hi_tgi);
  tgi_init ();
  tgi_setcolor(TGI_COLOR_WHITE);

  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    int len = strlen(buf);
    *strchr(buf, '\n') = 0;
    len--;
    if (len == 0) {
      break;
    }

    for (x = 0; x < max_x; x++) {
      if (len <= x || buf[x] == ' ') {
        bool_array_set(empty, x, y, 1);
        bool_array_set(tiles, x, y, 0);
      } else {
        int is_open = (buf[x] == '.');

        bool_array_set(empty, x, y, 0);
        bool_array_set(tiles, x, y, is_open);

        if (is_open) {
        } else {
          tgi_setpixel(x + x_offset, y + y_offset); /* rotate map */
        }
        if (is_open && start_x == -1) {
          start_x = x;
          start_y = y;
        }
      }
    }
    y++;
  }

  /* unit tests */
  if (!strcmp(DATASET, "IN22T")) {
    unit_tests_p1();
  }
  //unit_tests_p2();

  my_x = start_x;
  my_y = start_y;
  printf("Starting at (%d,%d) facing %d\n", my_x, my_y, my_direction);
  /* read the instructions */
  i = 0;
  while((c = fgetc(fp)) != '\n') {
    if (c == 'L' || c == 'R') {
      walk = atoi(buf);

      /* do the walk */
      do_walk(walk);
      do_turn(c);

      i = 0;
    } else {
      buf[i] = c;
      i++;
      buf[i] = '\0';
    }
  }
  free(buf);

  /* do the last walk */
  do_walk(walk);

#ifdef __CC65__
    tgi_done();
    clrscr();
#endif

  /* Done */
  printf("My long walk is finished.\n");
  printf("I'm at (%d,%d) facing %d.\n", my_x, my_y, my_direction);
  my_x++;
  my_y++;
  result = (long)(my_y * 1000L) + (long)(my_x * 4L) + (long)(my_direction);
  printf("(%d*1000 + %d*4 + %d) = %ld\n", my_y, my_x, my_direction, result);
  fclose(fp);
  bool_array_free(tiles);
  bool_array_free(empty);
  cgetc();
}

static void unit_tests_p1(void) {
  /* basic move */
  test(2, 2, 2, RIGHT, 4, 2, "go right");
  test(2, 2, 2, DOWN,  2, 4, "go down");
  test(2, 2, 2, LEFT,  0, 2, "go left");
  test(3, 2, 2, UP,    3, 0, "go up");
  
  /* wrap */
  test(8, 1, 5, RIGHT, 2, 1, "wrap right 1");
  test(1, 8, 5, DOWN, 1, 2, "wrap down 1");
  test(2, 1, 5, LEFT, 8, 1, "wrap left 1");
  test(1, 1, 5, UP, 1, 7, "wrap up 1");

  test(18,12, 5, RIGHT, 12, 12, "wrap right 2");
  test(12,19, 5, DOWN, 12, 13, "wrap down 2");
  test(13, 12, 5, LEFT, 19, 12, "wrap left 2");
  test(11, 12, 5, UP, 11, 18, "wrap up 2");
  
  /* obstacles, normal */
  test(2, 3, 5, UP, 2, 1, "obstacle up");
  test(2, 3, 5, RIGHT, 4, 3, "obstacle right");
  test(2, 3, 5, DOWN, 2, 4, "obstacle down");
  test(2, 3, 5, LEFT, 1, 3, "obstacle left");

  /* obstacles, wrapped 1 */
  test(8, 2, 5, UP, 8, 0, "obstacle up wrap");
  test(8, 3, 5, RIGHT, 10, 3, "obstacle right wrap");
  test(2, 8, 5, DOWN, 2, 10, "obstacle down wrap");
  test(2, 2, 5, LEFT, 0, 2, "obstacle left");
  
  /* obstacles, wrapped 2 */
  test(19, 14, 5, UP, 19, 11, "obstacle up wrap 2");
  test(19, 14, 5, RIGHT, 21, 14, "obstacle right wrap 2");
  test(13, 19, 5, DOWN, 13, 21, "obstacle down wrap 2");
  test(13, 13, 5, LEFT, 11, 13, "obstacle left wrap 2");
  exit(0);
}

static void unit_tests_p2(void) {
  /* basic move */
  test(50, 0, 1, UP, 0, 150, "go up from 2");
  test(99, 0, 1, UP, 0, 199, "go up from 2");

  test(100, 0, 1, UP, 0, 199, "go up from 1");
  test(149, 0, 1, UP, 49, 199, "go up from 1");

  test(50, 0, 1, LEFT, 0, 149, "go left from 2");
  test(50, 49, 1, LEFT, 0, 100, "go left from 2");

  test(149, 0, 1, RIGHT, 99, 149, "go right from 1");
  test(149, 49, 1, RIGHT, 99, 100, "go right from 1");

  test(102, 49, 1, DOWN, 99, 52, "go down from 1");
  test(148, 49, 1, DOWN, 99, 98, "go down from 1");

  test(50, 50, 1, LEFT, 0, 100, "go left from 3");
  test(50, 99, 1, LEFT, 49, 100, "go left from 3");

  test(99, 52, 1, RIGHT, 102, 49, "go right from 3");
  test(99, 99, 1, RIGHT, 149, 49, "go right from 3");

  test(0, 100, 1, UP, 50, 50, "go up from 5");
  test(49, 100, 1, UP, 50, 99, "go up from 5");

  test(0, 101, 1, LEFT, 50, 48, "go left from 5");
  test(0, 149, 1, LEFT, 50, 0, "go left from 5");

  test(99, 100, 1, RIGHT, 149, 49, "go right from 4");
  test(99, 149, 1, RIGHT, 149, 0, "go right from 4");

  test(51, 149, 1, DOWN, 49,151, "go down from 4");
  test(99, 149, 1, DOWN, 49, 199, "go down from 4");

  test(0, 150, 1, LEFT, 50, 0, "go left from 6");
  test(0, 199, 1, LEFT, 99, 0, "go left from 6");

  test(0, 199, 1, DOWN, 100, 0, "go down from 6");
  test(49, 199, 1, DOWN, 149, 0, "go down from 6");

  test(49, 151, 1, RIGHT, 51,149, "go right from 6");
  test(49, 199, 1, RIGHT, 99, 149, "go right from 6");

  cgetc();
}
