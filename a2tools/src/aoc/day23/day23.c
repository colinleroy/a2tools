#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#endif
#include "extended_conio.h"
#include "bool_array.h"

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

static int show_map;

static bool_array *cache = NULL;

static void dump_map(int round);

static int has_elf(int x, int y) {
  return x >= min_x && x < max_x &&
         y >= min_y && y < max_y &&
         bool_array_get(cache, x - min_x, y - min_y) != 0;
}

static int other_elves(elf *ref_elf, int direction) {
  switch(direction) {
    case NORTH:
      return has_elf(ref_elf->x - 1, ref_elf->y - 1)
          || has_elf(ref_elf->x,     ref_elf->y - 1)
          || has_elf(ref_elf->x + 1, ref_elf->y - 1);
    case EAST:
      return has_elf(ref_elf->x + 1, ref_elf->y - 1)
          || has_elf(ref_elf->x + 1, ref_elf->y)
          || has_elf(ref_elf->x + 1, ref_elf->y + 1);
    case SOUTH:
      return has_elf(ref_elf->x - 1, ref_elf->y + 1)
          || has_elf(ref_elf->x,     ref_elf->y + 1)
          || has_elf(ref_elf->x + 1, ref_elf->y + 1);
    case WEST:
      return has_elf(ref_elf->x - 1, ref_elf->y - 1)
          || has_elf(ref_elf->x - 1, ref_elf->y)
          || has_elf(ref_elf->x - 1, ref_elf->y + 1);
  }
  printf("unbelievable.\n");
  exit(0);
}

static FILE *elvesfp = NULL;
static void save_elves(void) {
  elvesfp = fopen("ELVES","w+b");
  fwrite(elves, sizeof(elf), num_elves, elvesfp);
}

static void read_elves(void) {
  elves = malloc(num_elves * sizeof(elf));
  fseek(elvesfp, 0, SEEK_SET);
  fread(elves, sizeof(elf), num_elves, elvesfp);
  fclose(elvesfp);
  elvesfp = NULL;
}

static void build_cache(int round) {
  elf *e;
  int i;
  cache = bool_array_alloc(max_x - min_x, max_y - min_y);

  if (!show_map) {
    e = malloc(sizeof(elf));
  }
  if (!show_map) {
    fseek(elvesfp, 0, SEEK_SET);
  }
  for (i = 0; i < num_elves; i++) {
    if (!show_map) {
      fread(e, sizeof(elf), 1, elvesfp);
    } else {
      e = &elves[i];
    }
    bool_array_set(cache, e->x - min_x, e->y - min_y, 1);
  }
  dump_map(round);
  
  if (!show_map) {
    free(e);
  }
}

static void plan_move(int num) {
  elf *e;
  int free_dirs[4];
  int i, planned_dir;
  
  if (!show_map) {
    e = malloc(sizeof(elf));
    fseek(elvesfp, num * sizeof(elf), SEEK_SET);
    fread(e, sizeof(elf), 1, elvesfp);
  } else {
    e = &elves[num];
  }

  free_dirs[NORTH] = !other_elves(e, NORTH);
  free_dirs[EAST]  = !other_elves(e, EAST);
  free_dirs[SOUTH] = !other_elves(e, SOUTH);
  free_dirs[WEST]  = !other_elves(e, WEST);

  if (free_dirs[NORTH] == 1 && free_dirs[EAST] == 1 &&
      free_dirs[SOUTH] == 1 && free_dirs[WEST] == 1) {
    if (!show_map) free(e);
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
  if (!show_map) free(e);
  return;
update_plan:
  switch(planned_dir) {
    case NORTH: e->p_y--; break;
    case SOUTH: e->p_y++; break;
    case EAST:  e->p_x++; break;
    case WEST:  e->p_x--; break;
  }

  if (!show_map) {
    /* save elf */
    fseek(elvesfp, num * sizeof(elf), SEEK_SET);
    fwrite(e, sizeof(elf), 1, elvesfp);
    free(e);
  }
}

static int num_elf_moved = 0;

static void execute_move(int elf) {
  int i, move_cancelled = 0;

  if (elves[elf].p_x == elves[elf].x && elves[elf].p_y == elves[elf].y)
    return;

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
    num_elf_moved++;
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

static void do_round(int round) {
  int i;
  show_map = max_x < 20;

  if (!show_map) printf(" Building map...\n");

  if (!show_map) {
    save_elves();
    free(elves);
  }
  build_cache(round);

  if (!show_map) printf(" Planning round...");
  for (i = 0; i < num_elves; i++) {
    plan_move(i);
  }

  if (!show_map) {
    printf("\n Freeing map and loading elves...");
    bool_array_free(cache);
    cache = NULL;
    read_elves();
  } else  {
    bool_array_free(cache);
  }
  if (!show_map) printf("\n Executing round...");
  for (i = 0; i < num_elves; i++) {
    execute_move(i);
  }
  if (!show_map) printf("\n %d elves moved.\n", num_elf_moved);
  prio_move = (prio_move + 1) % 4;
}

static int free_tiles;
static void dump_map(int round) {
  int x, y, full;
  int show = max_x < 20;
  free_tiles = 0;

  for (y = min_y; y < max_y; y++) {
    if (show) gotoxy(20 - max_x/2 + min_x, y + 8 - max_y/2 + min_y);
    for(x = min_x; x < max_x; x++) {
      full = has_elf(x, y);
      if (show) printf("%c", full ? '*':' ');
      if (!full) {
        free_tiles++;
      }
    }
    if (show) printf("\n");
  }

  if (round  == 10) {
    printf("\nWe have %d empty tiles after round 10.\n", free_tiles);
    printf("Hit a key to continue.");
    cgetc();
    clrscr();
  }
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

  clrscr();
  printfat(0, 21, 1, "Starting.\n");

  do {
    num_elf_moved = 0;
    
    do_round(round);
    printfat(0, 21, 1, "Finished round %d.\n", round + 1);
    round++;
  } while (num_elf_moved);

  printfat(0, 22, 1, "We did %d rounds.\n", round);
  free(elves);

  fclose(fp);
}
