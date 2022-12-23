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

#define DATASET "IN23"

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

#define NORTH 0
#define SOUTH 1
#define WEST  2
#define EAST  3

static int prio_move = NORTH;

typedef struct _elf {
  short x;
  short y;
  short p_x;
  short p_y;
} elf;

static elf *elves = NULL;
static int num_elves = 0;
static int min_x = 0, min_y = 0, max_x = 0, max_y = 0;

static int has_elf(int x, int y) {
  /* Fixme do I have room to optimize that */
  int i;
  for (i = 0; i < num_elves; i++) {
    if (elves[i].x == x && elves[i].y == y)
      return 1;
  }
  return 0;
}

static int other_elves(int ref_elf, int direction) {
  switch(direction) {
    case NORTH:
      return has_elf(elves[ref_elf].x - 1, elves[ref_elf].y - 1)
          || has_elf(elves[ref_elf].x,     elves[ref_elf].y - 1)
          || has_elf(elves[ref_elf].x + 1, elves[ref_elf].y - 1);
    case EAST:
      return has_elf(elves[ref_elf].x + 1, elves[ref_elf].y - 1)
          || has_elf(elves[ref_elf].x + 1, elves[ref_elf].y)
          || has_elf(elves[ref_elf].x + 1, elves[ref_elf].y + 1);
    case SOUTH:
      return has_elf(elves[ref_elf].x - 1, elves[ref_elf].y + 1)
          || has_elf(elves[ref_elf].x,     elves[ref_elf].y + 1)
          || has_elf(elves[ref_elf].x + 1, elves[ref_elf].y + 1);
    case WEST:
      return has_elf(elves[ref_elf].x - 1, elves[ref_elf].y - 1)
          || has_elf(elves[ref_elf].x - 1, elves[ref_elf].y)
          || has_elf(elves[ref_elf].x - 1, elves[ref_elf].y + 1);
  }
  printf("unbelievable.\n");
  exit(0);
}

static void plan_move(int elf) {
  int free_dirs[4];
  int i, planned_dir;
  free_dirs[NORTH] = !other_elves(elf, NORTH);
  free_dirs[EAST]  = !other_elves(elf, EAST);
  free_dirs[SOUTH] = !other_elves(elf, SOUTH);
  free_dirs[WEST]  = !other_elves(elf, WEST);

  elves[elf].p_x = elves[elf].x;
  elves[elf].p_y = elves[elf].y;

  if (free_dirs[NORTH] == 1 && free_dirs[EAST] == 1 &&
      free_dirs[SOUTH] == 1 && free_dirs[WEST] == 1) {
    return;
  }

  for (i = 0; i < 4; i++) {
    planned_dir = (prio_move + i) % 4;
    if (free_dirs[planned_dir]) {
      /* this one is free */
      goto update_plan;
    }
  }
  /* else there is no free direction */
  return;
update_plan:
  switch(planned_dir) {
    case NORTH: elves[elf].p_y--; break;
    case SOUTH: elves[elf].p_y++; break;
    case EAST:  elves[elf].p_x++; break;
    case WEST:  elves[elf].p_x--; break;
  }
}

static int any_elf_moved = 0;

static void execute_move(int elf) {
  int i, move_cancelled = 0;
  for (i = 0; i < num_elves; i++) {
    if (i == elf) {
      continue;
    } else if (elves[i].p_x == elves[elf].p_x && elves[i].p_y == elves[elf].p_y) {
      /* got to cancel all p_x, p_y moves */
      move_cancelled = 1;
      elves[i].p_x = elves[i].x;
      elves[i].p_y = elves[i].y;
    }
  }
  if (move_cancelled) {
    /* and cancel mine */
    elves[elf].p_x = elves[elf].x;
    elves[elf].p_y = elves[elf].y;
  } else {
    any_elf_moved++;
    elves[elf].x = elves[elf].p_x;
    elves[elf].y = elves[elf].p_y;

    /* update bounds */
    if (elves[elf].x < min_x) {
      min_x = elves[elf].x;
    }
    if (elves[elf].y < min_y) {
      min_y = elves[elf].y;
    }
    if (elves[elf].x >= max_x) {
      max_x = elves[elf].x + 1;
    }
    if (elves[elf].y >= max_y) {
      max_y = elves[elf].y + 1;
    }
  }
}

static void do_round(void) {
  int i;

  printf(" Planning round...\n");
  for (i = 0; i < num_elves; i++) {
    plan_move(i);
  }

  printf(" Executing round...\n");
  for (i = 0; i < num_elves; i++) {
    execute_move(i);
  }
  printf(" %d elves moved.\n", any_elf_moved);
  prio_move = (prio_move + 1) % 4;
}

static int free_tiles;
static void dump_map(void) {
  int x, y, full;
  free_tiles = 0;

  for (y = min_y; y < max_y; y++) {
    for(x = min_x; x < max_x; x++) {
      full = has_elf(x, y);
      //printf("%c", full ? '#':'.');
      if (!full) {
        free_tiles++;
      }
    }
    //printf("\n");
  }
  //cgetc();
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  char *c;
  int i, round = 0;
  int x = 0, y = 0;

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

  printf("Starting.\n");
  //dump_map();

  do {
    any_elf_moved = 0;
    do_round();
    printf("Finished round %d.\n", round + 1);
    if (round + 1 == 10) {
      dump_map();
      printf("We have %d empty tiles.\n", free_tiles);
    }
    round++;
  } while (any_elf_moved);

  printf("We did %d rounds.\n", round);
  free(elves);

  fclose(fp);
}
