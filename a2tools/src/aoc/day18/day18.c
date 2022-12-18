#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
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


int min_x = 0, min_y = 0, min_z = 0;
int max_x = 0, max_y = 0, max_z = 0;

static int is_outside_reachable(char ***cubes, int x, int y, int z, int d) {
  char *prefix;
  int i, num_unsure_neighbors = 0, result = UNSURE;

  if (!IS_EMPTY(cubes[x][y][z]))
    return UNREACHABLE;
  if (CAN_REACH_OUTSIDE(cubes[x][y][z]) == REACHABLE)
    return REACHABLE;
  if (CAN_REACH_OUTSIDE(cubes[x][y][z]) == UNREACHABLE)
    return UNREACHABLE;
  
  prefix = malloc(d + 1);
  for (i = 0; i < d; i++) {
    prefix[i] = ' ';
  }
  prefix[i] = '\0';
  printf("%scan %d,%d,%d reach outside?\n", prefix, x, y, z);
  if (x == min_x || x == max_x
   || y == min_y || y == max_y
   || z == min_z || z == max_z) {
     cubes[x][y][z] &= ~UNSURE;
     cubes[x][y][z] |= REACHABLE;
     printf("%syes %d,%d,%d can reach outside because on border\n", prefix, x, y, z);
     result = REACHABLE;
     goto out;
   }
  cubes[x][y][z] |= INVESTIGATING;
  
  if (!IS_INVESTIGATING(cubes[x-1][y][z])) {
    result = is_outside_reachable(cubes, x-1, y, z, d + 1);
    if (result == REACHABLE)
      goto yes;
    else if (result == UNSURE)
      num_unsure_neighbors++;
  }
  if (!IS_INVESTIGATING(cubes[x+1][y][z])) {
    result = is_outside_reachable(cubes, x+1, y, z, d + 1);
    if (result == REACHABLE)
      goto yes;
    else if (result == UNSURE)
      num_unsure_neighbors++;
  }
  if (!IS_INVESTIGATING(cubes[x][y-1][z])) {
    result = is_outside_reachable(cubes, x, y-1, z, d + 1);
    if (result == REACHABLE)
      goto yes;
    else if (result == UNSURE)
      num_unsure_neighbors++;
  }
  if (!IS_INVESTIGATING(cubes[x][y+1][z])) {
    result = is_outside_reachable(cubes, x, y+1, z, d + 1);
    if (result == REACHABLE)
      goto yes;
    else if (result == UNSURE)
      num_unsure_neighbors++;
  }
  if (!IS_INVESTIGATING(cubes[x][y][z])) {
    result = is_outside_reachable(cubes, x, y, z, d + 1);
    if (result == REACHABLE)
      goto yes;
    else if (result == UNSURE)
      num_unsure_neighbors++;
  }
  if (!IS_INVESTIGATING(cubes[x][y][z-1])) {
    result = is_outside_reachable(cubes, x, y, z-1, d + 1);
    if (result == REACHABLE)
      goto yes;
    else if (result == UNSURE)
      num_unsure_neighbors++;
  }
  if (!IS_INVESTIGATING(cubes[x][y][z+1])) {
    result = is_outside_reachable(cubes, x, y, z+1, d + 1);
    if (result == REACHABLE)
      goto yes;
    else if (result == UNSURE)
      num_unsure_neighbors++;
  }

  if (num_unsure_neighbors > 0) {
     cubes[x][y][z] &= UNSURE;
     printf("%sunsure %d,%d,%d at least one neighbor is unsure\n", prefix, x, y, z);
     result = REACHABLE;
     goto out;
   } else {
     cubes[x][y][z] &= ~UNSURE;
     cubes[x][y][z] |= UNREACHABLE;
     printf("%sno %d,%d,%d can't reach outside because no neighbor can\n", prefix, x, y, z);
     result = UNREACHABLE;
     goto out;
   }
   /* Should never be there */
   exit(1);
yes:
   cubes[x][y][z] &= ~UNSURE;
   cubes[x][y][z] |= REACHABLE;
   printf("%syes %d,%d,%d can reach outside because a neighbor can\n", prefix, x, y, z);

out:

  cubes[x][y][z] &= ~INVESTIGATING;

  free(prefix);
  return result;
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  int num_cubes = 0, i, j;
  char ***cubes = NULL;
  int num_faces = 0, num_connected_faces = 0;
  char *s_x, *s_y, *s_z;
  int x, y, z;
  int num_empty_cubes_that_reach_outside = 0;
  int num_empty_cubes_unsure_reach_outside = 0;
  int num_empty_cubes_that_cant_reach_outside = 0;
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
    
    if (x < min_x) min_x = x;
    if (y < min_y) min_y = y;
    if (z < min_z) min_z = z;

    if (x > max_x) max_x = x;
    if (y > max_y) max_y = y;
    if (z > max_z) max_z = z;

    num_cubes++;
    gotoxy(0,0);
    // printf("Read %d cubes, map size %d*%d*%d\n",
    //        num_cubes, max_x, max_y, max_z);
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
      memset(cubes[i][j], EMPTY, (max_y + 1) * sizeof(char *));
    }
  }
  rewind(fp);

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
    
    cubes[x][y][z] = FULL;

    gotoxy(0,0);
    //printf("Building map at %d,%d,%d           \n", x, y, z);

    num_faces += 6;

    if (x > 0)     num_connected_faces += ((int)cubes[x-1][y][z] * 2);
    if (x < max_x) num_connected_faces += ((int)cubes[x+1][y][z] * 2);
    if (y > 0)     num_connected_faces += ((int)cubes[x][y-1][z] * 2);
    if (y < max_y) num_connected_faces += ((int)cubes[x][y+1][z] * 2);
    if (z > 0)     num_connected_faces += ((int)cubes[x][y][z-1] * 2);
    if (z < max_z) num_connected_faces += ((int)cubes[x][y][z+1] * 2);
  }

  /* compute which empty places can't reach outside */
  for (x = 0; x <= max_x; x++) {
    for (y = 0; y <= max_y; y++) {
      for (z = 0; z <= max_z; z++) {
        is_outside_reachable(cubes, x, y, z, 0);
      }
    }
  }
fill_in_results:
  for (x = 0; x <= max_x; x++) {
    for (y = 0; y <= max_y; y++) {
      for (z = 0; z <= max_z; z++) {
        if (!IS_EMPTY(cubes[x][y][z]))
          continue;
        if (CAN_REACH_OUTSIDE(cubes[x][y][z]) == REACHABLE)
          num_empty_cubes_that_reach_outside ++;
        else if (CAN_REACH_OUTSIDE(cubes[x][y][z])== UNREACHABLE)
          num_empty_cubes_that_cant_reach_outside ++;
        else if (CAN_REACH_OUTSIDE(cubes[x][y][z]) == UNSURE
               ||CAN_REACH_OUTSIDE(cubes[x][y][z]) == 0) {
          /* check again */
          printf("Rechecking %d,%d,%d\n", x, y, z);
          if (is_outside_reachable(cubes, x, y, z, 0) == UNSURE) {
            printf(" Bug! cube at %d,%d,%d is still empty unsure\n", x, y, z);
            goto fill_in_results;
          } else if (CAN_REACH_OUTSIDE(cubes[x][y][z]) == REACHABLE) {
            num_empty_cubes_that_reach_outside ++;
          } else if (CAN_REACH_OUTSIDE(cubes[x][y][z]) == UNREACHABLE) {
            num_empty_cubes_that_reach_outside ++;
          } else {
            exit(1);
            printf(" Bug! cube at %d,%d,%d is unchecked\n", x, y, z);
            goto fill_in_results;
          }
        }
        else if (IS_INVESTIGATING(cubes[x][y][z]))
          printf(" Bug! cube at %d,%d,%d is still investigated\n", x, y, z);
      }
    }
  }
  free(buf);
  fclose(fp);

  printf("\n");
  printf("Map has %d cubes, %d faces,\n"
         "%d connected, %d unconnected,\n"
         "%d empty spots connected to outside\n"
         "%d empty spots unsure if connected to outside\n"
         "%d empty spots not connected to outside\n",
         num_cubes, num_faces,
         num_connected_faces, num_faces - num_connected_faces,
         num_empty_cubes_that_reach_outside,
         num_empty_cubes_unsure_reach_outside,
         num_empty_cubes_that_cant_reach_outside);

  for (i = 0; i < max_x + 1; i++) {
    for (j = 0; j < max_y + 1; j++) {
      free(cubes[i][j]);
    }
    free(cubes[i]);
  }
  free(cubes);

}
