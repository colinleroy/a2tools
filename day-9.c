#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
short x[N_KNOTS], y[N_KNOTS];
int n_mov = 0;
int lnum = 0;
int n_visited = 0;
int minx = 1, maxx = 0, miny = 1, maxy = 0, xlen = 0, ylen = 0, sim = 1, xoff = 0, yoff = 0;
char *visited = NULL, visited_len = 0, rem_visited_len = 0;

static int check_visited(short x, short y, int set) {
  int offset = ((ylen + 1) * (x+xoff-1)) + (y+yoff-1);
  int byte = 1 + (offset / 8);
  int bit = offset % 8;
  
  if (set) {
    visited[byte] |= (1 << bit);
  }
  return (visited[byte] & (1 << bit)) != 0;
}

static void update_boundaries(int x, int y) {
  if (x < minx) minx = x;
  if (x > maxx) maxx = x;
  if (y < miny) miny = y;
  if (y > maxy) maxy = y;
}

static void handle_line() {
  dx = get_dx(dir);
  dy = get_dy(dir);
  delta = atoi(s_num);
  int i,t;
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
          check_visited(x[t], y[t], 1);
        }
      }
    }
  }
  if (lnum %10 == 0) {
    printf("handled line %d (%c %d)\n", lnum, dir, delta);
  }
  lnum++;
}
int main(int argc, char **argv) {
  sim = 1;

again:
  n_mov = 0;
  lnum = 0;
  if (sim == 0) {
    n_visited += check_visited(1, 1, 1);
  }
  for (int t = 0; t < N_KNOTS; t++) {
    x[t] = 1; y[t] = 1;
  }
  while ((c = fgetc(stdin)) != EOF) {    
    switch (parse_step) {
      case 0:
        dir = c;
        if (c == '\n') {
          goto end_sim;
        }
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
end_sim:
  if (sim == 1) {
    sim = 0;
    xlen = abs(maxx - minx);
    ylen = abs(maxy - miny);
    xoff = -minx + 1;
    yoff = -miny + 1;
    visited = malloc(((xlen+2)*(ylen+2))/8);
    memset(visited, 0, ((xlen+2)*(ylen+2))/8);
    goto again;
  } else {
    int i, j;
    n_visited = 0;
    for (i = minx; i <= maxx; i++) {
      for (j = miny; j <= maxy; j++) {
        n_visited += check_visited(i,j,0);
      }
    }
  }
  printf("nvisited: %d\n", n_visited);
}
