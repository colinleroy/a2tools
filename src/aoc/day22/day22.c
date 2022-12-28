#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#endif
#include "tgi_compat.h"
#include "extended_conio.h"
#include "bool_array.h"

#define BUFSIZE 255
static void read_file(FILE *fp);

static void unit_tests(void);

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

static int next_tile(int x, int y, char direction) {
  int dx, dy;
  switch (direction) {
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
    return (direction == UP || direction == DOWN) ? my_y : my_x;
  } else {
    return (direction == UP || direction == DOWN) ? y : x;
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
      switch(my_direction) {
        case UP:    my_y = next_tile(my_x, my_y, UP);    
          break;
        case RIGHT: my_x = next_tile(my_x, my_y, RIGHT); 
          break;
        case DOWN:  my_y = next_tile(my_x, my_y, DOWN);  
          break;
        case LEFT:  my_x = next_tile(my_x, my_y, LEFT);  
          break;
      }
      if (old_x == my_x && old_y == my_y) {
        return;
      }
      tgi_setcolor(TGI_COLOR_WHITE);
      tgi_setpixel(my_y + y_offset, my_x + x_offset);
    } else if (is_obstacle(my_x + dx, my_y + dy) && !is_empty(my_x + dx, my_y + dy)) {
      /* stop at obstacle */
      return;
    } else {
      /* walk */
      my_x += dx;
      my_y += dy;
      tgi_setcolor(TGI_COLOR_WHITE);
      tgi_setpixel(my_y + y_offset, my_x + x_offset);
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
    printf("%s FAILED (expected %d,%d, got %d,%d)\n", label, end_x, end_y, my_x, my_y);
  }
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  int x = 0, y = 0;
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

  x_offset = (192 - max_x) / 2;
  y_offset = (280 - max_y) / 2;

  tgi_install(a2_hi_tgi);
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
          tgi_setpixel(y + y_offset, x + x_offset); /* rotate map */
        }
        if (is_open && my_x == -1) {
          my_x = x;
          my_y = y;
        }
      }
    }
    y++;
  }

  /* unit tests */
  if (!strcmp(DATASET, "IN22T")) {
    unit_tests();
  }

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

static void unit_tests(void) {
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
