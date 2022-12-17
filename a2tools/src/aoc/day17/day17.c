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
int shape_heights[5] = {4, 3, 3, 4, 2};

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


#define VIZ
#ifdef VIZ
static char *blank_line = NULL;
static char *cutoff_line = NULL;
static char *bottom_line = NULL;
static void viz(char *msg, int iterations, bool_array *table, int cur_shape, int cur_shape_left, int cur_shape_bottom, int cur_wind, int floor_height) {
  int cur_shape_top = cur_shape_bottom + 4;
  int max_fh = floor_height;
  int line = max_fh + 10;
  int i, c;
  int x, y;

#ifdef __CC65__
  gotoxy(1, 1);
#endif
  // if (iterations != NUM_ITERATIONS) {
  //   if (iterations > 10 && iterations % 10)
  //     return;
  //   else if (iterations > 100 && iterations % 100)
  //     return;
  // }
  for (x = 0; x < WALLS_WIDTH; x++) {
    printf("%d", x);
  }
  printf("\n");
  for (y = max_fh + 7; y >= max_fh - 19 && y >= 0; y--) {
    for (x = 0; x < WALLS_WIDTH; x++) {
      if (y <= cur_shape_top - 1 && y > cur_shape_bottom - 1
       && x >= cur_shape_left && x < cur_shape_left + shape_widths[cur_shape]
       && shape_masks[cur_shape][cur_shape_top - y - 1][x - cur_shape_left]) {
        printf("#");
      } else if (bool_array_get(table, x, y)) {
        printf("@");
      } else {
        printf(".");
      }
    }
    printf(" (%d)\n", y);
  }
  printf("+%s+ (0) - %s (%d,%d)\n", bottom_line, msg, cur_shape_left, cur_shape_bottom);
  getc(stdin);
}
#else
#define viz(msg, iterations, table, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind) do {} while (0)
#endif

static int can_move_left(bool_array *table, int shape, int x, int y) {
  int i, d_x;
  if (x == 0) {
    return 0;
  }

  /* check each row */
  for (i = 0; i < shape_heights[shape]; i++) {
    /* shift to first block of shape at that line */
    int shape_left_start;
    for (shape_left_start = 0, d_x = x; 
         shape_left_start < shape_widths[shape] && d_x < WALLS_WIDTH
         && shape_masks[shape][3 - i][shape_left_start] == 0;
         shape_left_start++, d_x++);
    if (bool_array_get(table, d_x - 1, y) != 0) {
      return 0;
    }
  }
  return 1;
}

static int can_move_right(bool_array *table, int shape,int x, int y) {
  int i, d_x;
  if (x + shape_widths[shape] == WALLS_WIDTH) {
    return 0;
  }


  /* check each row */
  for (i = 0; i < shape_heights[shape]; i++) {
    /* shift to first block of shape at that line */
    int shape_right_start = 4 - shape_widths[shape];
    int d_x = x + shape_widths[shape];

    while (shape_right_start >= 0 && shape_masks[shape][3 - i][shape_right_start] == 0) {
      shape_right_start--;
      d_x--;
    }

    if (bool_array_get(table, d_x + 1, y) != 0) {
      return 0;
    }
  }
  return 1;
}

static int can_move_down(bool_array *table, int shape, int x, int y) {
  int i;

  /* check each column */
  for (i = 0; i < shape_widths[shape]; i++) {
    /* shift to first block of shape at that line */
    int shape_bottom_start = 3;
    int d_y = y;
    
    while (shape_bottom_start >= 0 && shape_masks[shape][shape_bottom_start][i] == 0) {
      shape_bottom_start--;
      d_y++;
    }
    
    if (bool_array_get(table, x + i, d_y - 1) != 0) {
      printf("can't move down! %d,%d is blocked\n", x, d_y-1);
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

  do {
    cur_shape_bottom = 4 + floor_height;
    cur_shape_left = 2;
    must_land = 0;
    viz("start round", 
          iterations, table, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height);

    while (1) {
      cur_wind = (bool_array_get(wind, wind_count, 0)) == 0;
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
  } while (iterations < NUM_ITERATIONS);
  
  viz("done!", 
        iterations, table, cur_shape, cur_shape_left, cur_shape_bottom, cur_wind, floor_height);

}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  bool_array *wind, *table;
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
