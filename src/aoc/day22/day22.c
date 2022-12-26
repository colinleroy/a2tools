#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#include <tgi.h>
#endif
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
static char *directions[4] = {
  "Right",
  "Down",
  "Left",
  "Up"
};

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

#define X_RES 280
#define Y_RES 192
#define X_RES_HALF 140
#define Y_RES_HALF 96

#if 0
static int cur_draw_page = 1;
static int num_pages = 1;

static void draw_map(int x, int y) {
  int last_color = -1, color, r, c;
  int s_x, s_y, map_x, map_y;
  if (num_pages < 0) {
    return;
  }
  if (num_pages > 1) {
    tgi_setdrawpage(cur_draw_page);
  }
  
  for (r = 0; r < Y_RES; r++) {
    for (c = 0; c < X_RES; c++) {
      map_x = x - X_RES_HALF + c;
      map_y = y - Y_RES_HALF + r;
      s_x = map_x - x + X_RES_HALF;
      s_y = map_y - y + Y_RES_HALF;
      if (map_x < 0 || map_x >= max_x || map_y < 0 || map_y >= max_y
       || is_empty(map_x, map_y)) {
        color = TGI_COLOR_BLACK;
      } else if (is_obstacle(map_x, map_y)) {
        color = TGI_COLOR_WHITE;
      } else if (map_x != x && map_y != y) {
        color = TGI_COLOR_ORANGE;
      } else {
        color = TGI_COLOR_WHITE;
      }
      if (color != last_color) {
        tgi_setcolor(color);
        last_color = color;
      }
      tgi_setpixel(s_x, s_y);
    }
  }
  if (num_pages > 1) {
    tgi_setviewpage(cur_draw_page);
    cur_draw_page = 1 - cur_draw_page;
  }
}
#else
static void draw_map(int x, int y) {
}
#endif

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
      draw_map(my_x, my_y);
    } else if (is_obstacle(my_x + dx, my_y + dy) && !is_empty(my_x + dx, my_y + dy)) {
      /* stop at obstacle */
      return;
    } else {
      /* walk */
      draw_map(my_x + dx, my_y + dy);
      my_x += dx;
      my_y += dy;
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

#if 0
static void setup_tgi(void) {
  char tgi_e;
  tgi_install(a2_lo_tgi);
  if ((tgi_e = tgi_geterror()) != TGI_ERR_OK) {
    printf("error %d installing tgi\n", tgi_e);
    num_pages = -1;
    goto tgi_err;
  }

  tgi_init ();
  if ((tgi_e = tgi_geterror()) != TGI_ERR_OK) {
    printf("error %d initializing tgi\n", tgi_e);
    num_pages = -1;
    goto tgi_err;
  }

  num_pages = tgi_getpagecount();
  if ((tgi_e = tgi_geterror()) != TGI_ERR_OK) {
    printf("error %d getting page count\n", tgi_e);
    num_pages = -1;
    goto tgi_err;
  }

  tgi_setdrawpage(0);
  if ((tgi_e = tgi_geterror()) != TGI_ERR_OK) {
    printf("error %d setting draw page\n", tgi_e);
    num_pages = -1;
    goto tgi_err;
  }
  tgi_clear();
  if ((tgi_e = tgi_geterror()) != TGI_ERR_OK) {
    printf("error %d clearing screen\n", tgi_e);
    num_pages = -1;
    goto tgi_err;
  }
  tgi_setviewpage(0);
  if ((tgi_e = tgi_geterror()) != TGI_ERR_OK) {
    printf("error %d setting view page\n", tgi_e);
    num_pages = -1;
    goto tgi_err;
  }

  if (num_pages > 1) {
    tgi_setdrawpage(1);
    tgi_clear();
  }
  return;

tgi_err:
  tgi_done();
}
#endif

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  int x = 0, y = 0;
  char c;
  int i, walk;
#ifdef __CC65__
  unsigned long result;
#else
  unsigned int result;
#endif

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

#if 0
  setup_tgi();
#endif

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

  printf("Starting at (%d,%d) facing %s\n", my_x, my_y, directions[my_direction]);
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
  printf("I'm at (%d,%d) facing %s.\n", my_x, my_y, directions[my_direction]);
  my_x++;
  my_y++;
  result = (long)(my_y * 1000) + (long)(my_x * 4) + (long)(my_direction);
  printf("(%d*1000 + %d*4 + %d) = %d\n", my_y, my_x, my_direction, result);
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
