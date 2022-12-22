#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#endif
#include "bool_array.h"

#define BUFSIZE 255
static void read_file(FILE *fp);

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

/* 1 = walkable, 0 = obstacle */
static bool_array *tiles;
/* 1 = empty, 0 = is tile */
static bool_array *empty;

int max_x = 0, max_y = 0;

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  int x = 0, y = 0;
  char c;
  int i, walk;
 
  /* get map size */
  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
      int len = strlen(buf);

      *strchr(buf, '\n') = 0;
      len--;
      printf("line len %d\n", len);
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
  
  /* fill in map */
  tiles = bool_array_alloc(max_x, max_y);
  empty = bool_array_alloc(max_x, max_y);
  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
      int len = strlen(buf);
      *strchr(buf, '\n') = 0;
      len--;
      if (len == 0) {
          break;
      }
      printf("reading %d\n", y);
      for (x = 0; x < max_x; x++) {
        if (len < x || buf[x] == ' ') {
            bool_array_set(empty, x, y, 1);
            bool_array_set(tiles, x, y, 0);
        } else {
            bool_array_set(empty, x, y, 0);
            bool_array_set(tiles, x, y, (buf[x] == '.'));
        }
      }
      y++;
  }

  /* read the instructions */
  i = 0;
  while((c = fgetc(fp)) != EOF) {
      if (c == 'L' || c == 'R') {
          walk = atoi(buf);

          /* do the walk */
          printf("walking %d then turning %c\n", walk, c);

          i = 0;
      } else {
          buf[i] = c;
          i++;
          buf[i] = '\0';
      }
  }
  free(buf);

  /* do the last walk */
  printf("walking %d then I'm done\n", walk);

  fclose(fp);
  bool_array_free(tiles);
  bool_array_free(empty);
}
