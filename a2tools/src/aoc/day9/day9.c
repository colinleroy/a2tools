#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <conio.h>
#include <string.h>
#include <dbg.h>
#include "bool_array.h"

#define HEAD 0
#define N_KNOTS 10

static int get_dx(char dir) {
  if (dir == 'L')
    return -1;
  else if (dir == 'R')
    return 1;
  return 0;
}
static int get_dy(char dir) {
  if (dir == 'D')
    return -1;
  else if (dir == 'U')
    return 1;
  return 0;
}

int nmove = 0;
char c;
int parse_step = 0;
char dir;
char s_num[4];
int cur_num = 0;
int delta, dx, dy, wx, wy, xdif, ydif;
int x[N_KNOTS], y[N_KNOTS];
int n_mov = 0;
int lnum = 0;
int n_visited = 0;
int minx = 1, maxx = 0, miny = 1, maxy = 0, sim = 1, xoff = 0, yoff = 0;
size_t xlen = 0, ylen = 0;
bool_array *visited = NULL;

static void update_boundaries(int x, int y) {
  if (x < minx) minx = x;
  if (x > maxx) maxx = x;
  if (y < miny) miny = y;
  if (y > maxy) maxy = y;
}

static void handle_line() {
  int i,t;
  dx = get_dx(dir);
  dy = get_dy(dir);
  delta = atoi(s_num);

  for (i = 1; i <= delta; i++) {
    x[HEAD] += dx; y[HEAD] += dy;

    /* calculate tails moves */
    for (t = 1; t < N_KNOTS; t++) {
      wx = 0; wy = 0; 
      xdif = x[t-1] - x[t];
      ydif = y[t-1] - y[t];
      
      if (abs(xdif) >= 2 || abs(ydif) >= 2) {
        if (xdif != 0) {
          wx = xdif/abs(xdif);
        }
        if (ydif != 0) {
          wy = ydif/abs(ydif);
        }

        x[t] += wx;
        y[t] += wy;
        if (sim == 1) {
          update_boundaries(x[t], y[t]);
        }
        n_mov++;
        if (sim == 0 && t == N_KNOTS - 1) {
          bool_array_set(visited, x[t] + xoff - 1, y[t] + yoff - 1, 1);
        }
      }
    }
  }
  if (lnum %10 == 0) {
    printf("handled line %d (%c %d)\n", lnum, dir, delta);
  }
  lnum++;
}
int main(void) {
  int t,total_lines = 0;
  extern char a2_ssc;
  FILE *infp;
  sim = 1;

  _filetype = PRODOS_T_TXT;
  infp = fopen("IN9","r");

  if (infp == NULL) {
    printf("Error opening file: %s\n", strerror(errno));
  }
again:
  n_mov = 0;
  lnum = 0;
  if (sim == 0) {
    n_visited += bool_array_set(visited, 1 + xoff - 1, 1 + yoff - 1, 1);
  }
  for (t = 0; t < N_KNOTS; t++) {
    x[t] = 1; y[t] = 1;
  }
  printf("Starting run\n");
  while (1) {
    int r;
    
    r = fgetc(infp);
    if (r == EOF) {
      if (sim == 1) {
          printf("End of sim\n");
          goto end_loop;
      } else {
          printf("End of run\n");
          goto end_loop;
      }
    }
    c = (char)r;

    switch (parse_step) {
      case 0:
        dir = c;
        /* fallback intended */
      case 1:
        parse_step++;
        break;
      case 2:
        /* distance */
        if (c == '\n') {
          parse_step = 0;
          cur_num = 0;
          handle_line();
          continue;
        }
        s_num[cur_num] = c;
        cur_num++;
        s_num[cur_num] = '\0';
        break;
    }
  }
end_loop:
  if (sim == 1) {
    rewind(infp);
    sim = 0;
    xlen = abs(maxx - minx) + 1;
    ylen = abs(maxy - miny) + 1;
    xoff = -minx + 1;
    yoff = -miny + 1;
    total_lines = lnum;

    printf("Map is %d x %d, hit ENTER\n", xlen, ylen);
    visited = bool_array_alloc(xlen, ylen);
    printf("x %d to %d, y %d to %d\n", minx, maxx, miny, maxx);
    printf("shift x %d, y %d\n", xoff - 1, miny, yoff - 1 );
    cgetc();
    if (visited == NULL) {
      printf("Coudn't allocate array :(\n");
      exit(1);
    }
    goto again;
  } else {
    int i, j;
    n_visited = 0;
    for (i = minx; i <= maxx; i++) {
      printf("counting line %d...\n", i);
      for (j = miny; j <= maxy; j++) {
        if (bool_array_get(visited, i + xoff - 1, j + yoff - 1))
          n_visited ++;
      }
    }
  }
  printf("nvisited: %d\n", n_visited);
  fclose(infp);
  exit(0);
}
