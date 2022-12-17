#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#include <conio.h>
#endif
#include "bool_array.h"

#define BUFSIZE 10100

#define NUM_ITERATIONS 2022
#define WALLS_WIDTH 7

static void read_file(FILE *fp);

#define DATASET "IN17E"

char shape_masks[5][4][4] = {
  {"    ",
   "    ",
   "    ",
   "####"
 },
  {"    ",
   " #  ",
   "### ",
   " #  "
 },
  {"    ",
   "  # ",
   "  # ",
   "### "
 },
  {"#   ",
   "#   ",
   "#   ",
   "#   "
 },
  {"    ",
   "    ",
   "##  ",
   "##  "
 },
};
int shape_widths[5]     = {4, 3, 3, 1, 2};
int shape_heights[5][4] = {
 {1,1,1,1},
 {2,3,2,0},
 {1,1,3,0},
 {4,0,0,0},
 {2,2,0,0},
};
int shape_bottom_offsets[5][4] = {
 {0,0,0,0},
 {1,0,1,-1},
 {0,0,0,-1},
 {0,-1,-1,-1},
 {0,0,-1,-1},
};

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


static int min_floor_height(int *floor_height) {
  int i;
  int h = 0;
  for (i = 0; i < WALLS_WIDTH; i++) {
    if (floor_height[i] < h) {
      h = floor_height[i];
    }
  }
  return h;
}

static int max_floor_height(int *floor_height) {
  int i;
  int h = 0;
  for (i = 0; i < WALLS_WIDTH; i++) {
    if (floor_height[i] > h) {
      h = floor_height[i];
    }
  }
  return h;
}
#define VIZ
#ifdef VIZ
static char *blank_line = NULL;
static char *cutoff_line = NULL;
static char *bottom_line = NULL;
static void viz(char *msg, int iterations, int cur_shape, int cur_shape_left, int cur_shape_bottom, int cur_wind, int *floor_height) {
  int cur_shape_top = cur_shape_bottom + 4;
  int max_fh = max_floor_height(floor_height);
  int min_fh = min_floor_height(floor_height);
  int line = max_fh + 10;
  int i, c;

#ifdef __CC65__
  gotoxy(1, 1);
#endif
  if (iterations != NUM_ITERATIONS) {
    if (iterations > 10 && iterations % 10)
      return;
    else if (iterations > 100 && iterations % 100)
      return;
  }
  printf("|%s| (%d) Iter. %d\n", blank_line, max_fh + 10, iterations);
  printf("|%s| (%d) Height %d\n", blank_line, max_fh + 9, max_fh);
  for (i = max_fh + 8; i > min_fh; i--) {
    if (cur_shape_top < i) {
      printf("|%s| (%d)\n", blank_line, i);
    } else if (cur_shape_top >= i && cur_shape_bottom < i) {
      printf("|");
      for (c = 0; c < cur_shape_left; c++) {
        printf("%c", i > floor_height[c] ? ' ':'@');
      }
      for (c = 0; c < shape_widths[cur_shape]; c++) {
        printf("%c", shape_masks[cur_shape][cur_shape_top - i][c] == '#' ? '#':' ');
      }
      for (c = cur_shape_left + shape_widths[cur_shape]; c < WALLS_WIDTH; c++) {
        printf("%c", i > floor_height[c] ? ' ':'@');
      }
      printf("| (%d) %c\n", i, cur_wind == WIND_LEFT ? '<':'>');
    } else if (i <= cur_shape_bottom && i > max_fh) {
      printf("|%s| (%d)\n", blank_line, i);
    } else if (i <= max_fh && i >= max_fh - 5) {
      printf("|");
      for (c = 0; c < WALLS_WIDTH; c++) {
        if (i > floor_height[c])
          printf(" ");
        else
          printf("@");
      }
      printf("| (%d)\n", i);
    } else if (i < max_fh - 5) {
      printf("|%s| (%d-%d)\n", cutoff_line, i, min_fh + 1);
      break;
    }
  }
  printf("+%s+ (0) - %s\n", bottom_line, msg);
  getc(stdin);
}
#else
#define viz(msg, iterations, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height) do {} while (0)
#endif

static int intersect(int cur_shape, int cur_shape_left, int cur_shape_bottom, int **floor_height) {
  int i, intersected = 0;
  for (i = cur_shape_left; i < cur_shape_left + shape_widths[cur_shape]; i++) {
    if (shape_bottom_offsets[cur_shape][i - cur_shape_left] != -1) {
      if (cur_shape_bottom + shape_bottom_offsets[cur_shape][i - cur_shape_left] == (*floor_height)[i]) {
        intersected = 1;
        break;
      }
    }
  }
  if (intersected) {
    for (i = cur_shape_left; i < cur_shape_left + shape_widths[cur_shape]; i++) {
      (*floor_height)[i] = cur_shape_bottom + shape_heights[cur_shape][i - cur_shape_left];
    }
  }
  return intersected;
}

static void simulate_falls(bool_array *wind, int wind_pattern_length) {
  int cur_shape = 0;
  int iterations = 0;
  int *floor_height = malloc(WALLS_WIDTH * sizeof(int));
  int wind_count = 0;
  char cur_wind;
  int cur_shape_bottom, cur_shape_left;

  memset(floor_height, 0, WALLS_WIDTH * sizeof(int));

  do {
    cur_shape_bottom = 3 + max_floor_height(floor_height);
    cur_shape_left = 2;

    while (1) {
      cur_wind = (bool_array_get(wind, wind_count, 0)) == 0;
      /* get pushed */
      if (cur_wind == WIND_RIGHT 
       && cur_shape_left + shape_widths[cur_shape] < WALLS_WIDTH
       && floor_height[cur_shape_left + shape_widths[cur_shape]] < cur_shape_bottom + 1) {
        cur_shape_left++;
      } else if (cur_wind == WIND_LEFT 
              && cur_shape_left > 0
              && floor_height[cur_shape_left - 1] < cur_shape_bottom + 1)
        cur_shape_left--;

      viz(cur_wind == WIND_LEFT ? "left" : "right", 
            iterations, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height);

      /* change wind for next time */
      wind_count = (wind_count + 1) % wind_pattern_length;

      /* get down */
      if (!intersect(cur_shape, cur_shape_left, cur_shape_bottom, &floor_height))
        cur_shape_bottom--;
      else
        break;

      viz("down", 
            iterations, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height);
    }

    iterations++;
    cur_shape = (cur_shape + 1) % 5;
  } while (iterations < NUM_ITERATIONS);
  
  viz("done!", 
        iterations, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height);

  free(floor_height);
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  bool_array *wind;
  char *c;
  int i = 0;
  int wind_pattern_length = 0;

  fgets(buf, BUFSIZE-1, fp);

  fclose(fp);

  *(strchr(buf, '\n')) = '\0';
  wind_pattern_length = strlen(buf);

  wind = bool_array_alloc(wind_pattern_length, 1);
  c = buf;
  while (*c != '\n' && *c != '\0') {
    bool_array_set(wind, i++, 0, (*c == '<'));
    c++;
  }
  free(buf);

  blank_line = malloc((1 + WALLS_WIDTH) * sizeof(char));
  cutoff_line = malloc((1 + WALLS_WIDTH) * sizeof(char));
  bottom_line = malloc((1 + WALLS_WIDTH) * sizeof(char));
  for (i = 0; i < WALLS_WIDTH; i++) {
    blank_line[i] = ' ';
    cutoff_line[i] = '.';
    bottom_line[i] = '-';
  }
  blank_line[i] = '\0';
  cutoff_line[i] = '\0';
  bottom_line[i] = '\0';

  simulate_falls(wind, wind_pattern_length);
  bool_array_free(wind);
}
