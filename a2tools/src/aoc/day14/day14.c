#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <conio.h>
#include <apple2.h>
#include <tgi.h>
#endif
#include "slist.h"
#include "bool_array.h"
#include "extended_string.h"

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
#ifdef __CC65__
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
#ifdef __CC65__
  tgi_install(a2_hi_tgi);
  tgi_init ();
  tgi_setcolor(TGI_COLOR_BLACK);
  tgi_clear();
  tgi_setcolor(TGI_COLOR_WHITE);
#endif

  print_obstacles(obstacles);

#ifdef __CC65__
  tgi_setcolor(TGI_COLOR_ORANGE);
#endif
  while (sand_fall(count, obstacles)) {
    count ++;
  }

#ifdef __CC65__
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

  mid_screen_x = ((240 - map_w) / 2) + 1;
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

static void print_obstacles(bool_array *obstacles) {
  int x, y;

  for (y = 0; y <= map_h; y++) {
    for (x = 0; x <= map_w; x++) {
#ifdef __CC65__
      if (bool_array_get(obstacles, x, y)) {
        tgi_setpixel(x + mid_screen_x, y + mid_screen_y);
      }
#else
      printf("%c", (bool_array_get(obstacles, x, y)) ? '#':' ');
#endif
    }
#ifndef __CC65__
    printf(" %d\n", y);
#endif
  }
}

static void update_sand(int x, int y) {
#ifdef __CC65__
  if (x > 0 && y > 0) {
    tgi_setpixel(OFF_X(x) + mid_screen_x, OFF_Y(y) + mid_screen_y);
  }
#endif
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
#ifndef __CC65__
        printf("grain of sand %d stopped at (%d,%d)\n", num, x, y);
#endif
        update_sand(x, y);
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
