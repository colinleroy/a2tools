#include <stdio.h>
#include <stdlib.h>
#include <serial.h>
#include <conio.h>
#include <string.h>
#include <dbg.h>

#define HEAD 0
#define N_KNOTS 10

static const struct ser_params Params = {
    SER_BAUD_9600,     /* Baudrate */
    SER_BITS_8,         /* Number of data bits */
    SER_STOP_1,         /* Number of stop bits */
    SER_PAR_NONE,       /* Parity setting */
    SER_HS_HW           /* Type of handshake to use */
};

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
char *visited = NULL, visited_len = 0, rem_visited_len = 0;

static void *bool_array_alloc(int xlen, int ylen) {
    size_t tmp, allocsize;
    char *array = NULL;
    int asize = _heapmaxavail();

    tmp = 1 + (xlen+2)/8;
    allocsize = tmp*(ylen+3);

    array = malloc(allocsize);
    if (array == NULL) {
      return NULL;
    }
    memset(array, 0, allocsize);
    return array;
}

static int bool_array_access(char *array, int x, int y, int set, int val) {
  /* FIXME The offsets should be in the callers */
  long offset = ((ylen + 1) * (x+xoff-1)) + (y+yoff-1);
  size_t byte = 1 + (offset / 8);
  size_t bit = offset % 8;

  if (set) {
    if (val) {
      array[byte] |= (1 << bit);
    } else {
      array[byte] &= ~(1 << bit);
    }
  }
  return (array[byte] & (1 << bit)) != 0;
}

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
          bool_array_access(visited, x[t], y[t], 1, 1);
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
  int t,total_lines = 0;
  extern char a2_ssc;

  ser_install(&a2_ssc);
  ser_apple2_slot(2);
  ser_open (&Params);

  printf("int : %d\n", sizeof(int));
  printf("short : %d\n", sizeof(short));
  printf("size_t : %d\n", sizeof(size_t));
  printf("long : %d\n", sizeof(long));

  sim = 1;

again:
  n_mov = 0;
  lnum = 0;
  if (sim == 0) {
    n_visited += bool_array_access(visited, 1, 1, 1, 1);
  }
  for (t = 0; t < N_KNOTS; t++) {
    x[t] = 1; y[t] = 1;
  }
  printf("Starting run\n");
  while (1) {
    while (ser_get(&c) == SER_ERR_NO_DATA) {
      if (sim == 1) {
        if (kbhit()) {
          printf("End of sim\n");
          cgetc();
          goto end_loop;
        }
      } else {
        if (lnum == total_lines) {
          printf("End of run\n");
          goto end_loop;
        }
      }
    }
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
    sim = 0;
    xlen = abs(maxx - minx);
    ylen = abs(maxy - miny);
    xoff = -minx + 1;
    yoff = -miny + 1;
    total_lines = lnum;

    printf("Map is %d x %d, hit ENTER\n", xlen, ylen);
    cgetc();
    visited = bool_array_alloc(xlen, ylen);

    goto again;
  } else {
    int i, j;
    n_visited = 0;
    for (i = minx; i <= maxx; i++) {
      printf("counting line %d...\n", i);
      for (j = miny; j <= maxy; j++) {
        n_visited += bool_array_access(visited, i, j, 0, 0);
      }
    }
  }
  printf("nvisited: %d\n", n_visited);
  ser_uninstall();
  cgetc();
  exit(0);
}
