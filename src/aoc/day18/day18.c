#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "extended_conio.h"

#define BUFSIZE 255
static void read_file(FILE *fp);

#define DATASET "IN18"

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

#define EMPTY 0
#define FULL  1
#define IS_EMPTY(i)          ((i & (1<<0)) == 0)

#define INVESTIGATING        (1<<1)
#define IS_INVESTIGATING(i)  ((i & INVESTIGATING) != 0)

#define REACHABLE            (1<<2)
#define UNREACHABLE          (1<<3)
#define UNSURE               (REACHABLE | UNREACHABLE)
#define CAN_REACH_OUTSIDE(i) (i & (REACHABLE | UNREACHABLE))

#define ACCOUNTED_FOR        (1<<4)
#define IS_ACCOUNTED_FOR(i)  ((i & ACCOUNTED_FOR) != 0)

char max_x = 0, max_y = 0, max_z = 0;

static char is_outside_reachable(char ***cubes, int x, int y, int z, int d) {
  char result = UNSURE;

  if (!IS_EMPTY(cubes[x][y][z]))
    return UNREACHABLE;
  if (CAN_REACH_OUTSIDE(cubes[x][y][z]) == REACHABLE)
    return REACHABLE;
  if (CAN_REACH_OUTSIDE(cubes[x][y][z]) == UNREACHABLE)
    return UNREACHABLE;
  
  if (x == 0 || x >= max_x
   || y == 0 || y >= max_y
   || z == 0 || z >= max_z) {
     result = REACHABLE;
     goto yes;
   }

  if (d > 19) {
    /* don't overflow stack */
    return UNSURE;
  }

  cubes[x][y][z] |= INVESTIGATING;
  
  if (IS_EMPTY(cubes[x-1][y][z]) && !IS_INVESTIGATING(cubes[x-1][y][z])) {
    result = is_outside_reachable(cubes, x-1, y, z, d + 1);
    if (result == REACHABLE)
      goto yes;
  }
  if (IS_EMPTY(cubes[x+1][y][z]) && !IS_INVESTIGATING(cubes[x+1][y][z])) {
    result = is_outside_reachable(cubes, x+1, y, z, d + 1);
    if (result == REACHABLE)
      goto yes;
  }
  if (IS_EMPTY(cubes[x][y-1][z]) && !IS_INVESTIGATING(cubes[x][y-1][z])) {
    result = is_outside_reachable(cubes, x, y-1, z, d + 1);
    if (result == REACHABLE)
      goto yes;
  }
  if (IS_EMPTY(cubes[x][y+1][z]) && !IS_INVESTIGATING(cubes[x][y+1][z])) {
    result = is_outside_reachable(cubes, x, y+1, z, d + 1);
    if (result == REACHABLE)
      goto yes;
  }
  if (IS_EMPTY(cubes[x][y][z-1]) && !IS_INVESTIGATING(cubes[x][y][z-1])) {
    result = is_outside_reachable(cubes, x, y, z-1, d + 1);
    if (result == REACHABLE)
      goto yes;
  }
  if (IS_EMPTY(cubes[x][y][z+1]) && !IS_INVESTIGATING(cubes[x][y][z+1])) {
    result = is_outside_reachable(cubes, x, y, z+1, d + 1);
    if (result == REACHABLE)
      goto yes;
  }

  return UNREACHABLE;
   
yes:
  cubes[x][y][z] &= ~UNSURE;
  cubes[x][y][z] |= REACHABLE;
  cubes[x][y][z] &= ~INVESTIGATING;

  return result;
}

char num_neighbors(char ***cubes, int x, int y, int z) {
  char n = 0;

  if (x > 0) {
    n += (!IS_EMPTY(cubes[x-1][y][z]));
  }
  if (x < max_x) {
    n += (!IS_EMPTY(cubes[x+1][y][z]));
  }
  if (y > 0) {
    n += (!IS_EMPTY(cubes[x][y-1][z]));
  }
  if (y < max_y) {
    n += (!IS_EMPTY(cubes[x][y+1][z]));
  }
  if (z > 0) {
    n += (!IS_EMPTY(cubes[x][y][z-1]));
  }
  if (z < max_z) {
    n += (!IS_EMPTY(cubes[x][y][z+1]));
  }

  return n;
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  int i, j;
  char ***cubes = NULL;
  int num_cubes = 0;
  int surface_area = 0;
  int exterior_surface_area = 0;
  char *s_x, *s_y, *s_z;
  int x, y, z;

  clrscr();
  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    s_x = buf;

    s_y = strchr(s_x, ',') + 1;
    s_z = strchr(s_y, ',') + 1;
    *strchr(s_x, ',') = '\0';
    *strchr(s_y, ',') = '\0';

    x = atoi(s_x);
    y = atoi(s_y);
    z = atoi(s_z);
    
    if (x > max_x) max_x = x;
    if (y > max_y) max_y = y;
    if (z > max_z) max_z = z;

    num_cubes++;
    gotoxy(0,0);
    printf("Read %d cubes, map size %d*%d*%d\n",
           num_cubes, max_x, max_y, max_z);
  }

  /* X dimension */
  cubes = realloc(cubes, (max_x + 1) * sizeof(char ***));
  memset(cubes, 0, (max_x + 1) * sizeof(char ***));
  for (i = 0; i < max_x + 1; i++) {
    /* Y dimension per X*/
    cubes[i] = realloc(cubes[i], (max_y + 1) * sizeof(char **));
    memset(cubes[i], 0, (max_y + 1) * sizeof(char **));
    for (j = 0; j < max_y + 1; j++) {
      /* Z dimension per X, Y */
      cubes[i][j] = realloc(cubes[i][j], (max_z + 1) * sizeof(char *));
      memset(cubes[i][j], EMPTY, (max_z + 1) * sizeof(char *));
    }
  }
  rewind(fp);

  clrscr(); i = 0;
  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    s_x = buf;
    i++;
    s_y = strchr(s_x, ',') + 1;
    s_z = strchr(s_y, ',') + 1;
    *strchr(s_x, ',') = '\0';
    *strchr(s_y, ',') = '\0';

    x = atoi(s_x);
    y = atoi(s_y);
    z = atoi(s_z);
    
    cubes[x][y][z] = FULL;

    gotoxy(0,0);
    printf("Building map at %d,%d,%d (%d/%d)    \n", x, y, z, i, num_cubes);
    surface_area += 6;

    if (x > 0)     surface_area -= (cubes[x-1][y][z] * 2);
    if (x < max_x) surface_area -= (cubes[x+1][y][z] * 2);
    if (y > 0)     surface_area -= (cubes[x][y-1][z] * 2);
    if (y < max_y) surface_area -= (cubes[x][y+1][z] * 2);
    if (z > 0)     surface_area -= (cubes[x][y][z-1] * 2);
    if (z < max_z) surface_area -= (cubes[x][y][z+1] * 2);
  
  }
  free(buf);
  fclose(fp);

  /* compute which empty places can't reach outside */
  for (x = 0; x <= max_x; x++) {
    for (y = 0; y <= max_y; y++) {
      for (z = 0; z <= max_z; z++) {
        is_outside_reachable(cubes, x, y, z, 0);
      }
    }
  }
  exterior_surface_area = 0;

  clrscr();
  for (z = 0; z <= max_z; z++) {
    gotoxy(0, 0);
    printf("z %2d - x 012345678901234567890 y\n", z);
    for (y = 0; y <= max_y; y++) {
      printf("         ");
      for (x = 0; x <= max_x; x++) {
        if (IS_EMPTY(cubes[x][y][z])) {
          if (CAN_REACH_OUTSIDE(cubes[x][y][z]) == REACHABLE) {
            printf(" ");
            exterior_surface_area += num_neighbors(cubes, x, y, z);
          } else {
            printf(".");
          }
        } else {
          /* count the surface out of bounds */
          if (x == 0 || y == 0 || z == 0
           || x == max_x || y == max_y || z == max_z) {
            exterior_surface_area++;
          }
          printf("*");
        }
      }
      printf("  %d\n", y);
    }
  }
  
  printf("\n");
  printf("Map size %d*%d*%d\n", max_x, max_y, max_z);
  printf("Map has %d cubes,\n"
         "%d total surface area,\n"
         "%d exterior surface area.\n",
         num_cubes,
         surface_area,
         exterior_surface_area);

  for (i = 0; i < max_x + 1; i++) {
    for (j = 0; j < max_y + 1; j++) {
      free(cubes[i][j]);
    }
    free(cubes[i]);
  }
  free(cubes);
}
