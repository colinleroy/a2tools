#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#endif

#define BUFSIZE 255
static void read_file(FILE *fp);

#define DATASET "IN23E"

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

typedef struct _elf {
  short x;
  short y;
  short p_x;
  short p_y;
} elf;

static elf *elves = NULL;

static int num_elves = 0;
static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  char *c;
  int i, x = 0, y = 0, max_x = 0, max_y = 0;
  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    c = buf;
    x = 0;
    while (*c != '\n') {
      if (*c == '#') {
        num_elves++;
      }
      x++;
      c++;
      if (max_x < x) {
        max_x = x;
      }
    }
    max_y++;
  };
  printf("read map %d*%d with %d elves.\n", max_x, max_y, num_elves);

  rewind(fp);

  elves = malloc(num_elves * sizeof(elf));
  i = 0;

  y = 0;
  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    c = buf;
    x = 0;
    while (*c != '\n') {
      if (*c == '#') {
        elves[i].x = x;
        elves[i].y = y;
        elves[i].p_x = x;
        elves[i].p_y = y;
        i++;
      }
      x++;
      c++;
    }
    y++;
  }
  free(buf);

  for (i = 0; i < num_elves; i++) {
    printf("Elf %d: (%d,%d)\n", i, elves[i].x, elves[i].y);
  }

  printf("\n");
  free(elves);

  fclose(fp);
}
