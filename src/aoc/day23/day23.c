#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "extended_conio.h"
#include "bool_array.h"

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

#define NO_MOVE 4

static int prio_move = NORTH;

typedef struct _elf {
  short x;
  short y;
  char planned_dir;
} elf;

static elf *elves = NULL;
static int num_elves = 0;
static int min_x = 0, min_y = 0, max_x = 0, max_y = 0;

static int cache_elves = 1;

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

static void build_cache(int round) {
  elf *e;
  int i;
  cache = bool_array_alloc(max_x - min_x, max_y - min_y);

  if (!cache_elves) {
    e = malloc(sizeof(elf));
  }
  if (!cache_elves) {
    fseek(elvesfp, 0, SEEK_SET);
  }
  for (i = 0; i < num_elves; i++) {
    if (!cache_elves) {
      fread(e, sizeof(elf), 1, elvesfp);
    } else {
      e = &elves[i];
    }
    bool_array_set(cache, e->x - min_x, e->y - min_y, 1);
  }
  dump_map(round);
  
  if (!cache_elves) {
    free(e);
  }
}

static int prev_min_x, prev_min_y, prev_max_x, prev_max_y;

static short *dest_cache = NULL;

static void init_dest_cache(void) {
  int i;
  prev_min_x = min_x;
  prev_min_y = min_y;
  prev_max_x = max_x;
  prev_max_y = max_y;

  dest_cache = malloc((prev_max_x - prev_min_x + 2) * (prev_max_y - prev_min_y + 2) * sizeof(short));
  for (i = 0; i < (prev_max_x - prev_min_x + 2) * (prev_max_y - prev_min_y + 2); i++) {
    dest_cache[i] = -1;
  }
}

static void free_dest_cache(void) {
  free(dest_cache);
  dest_cache = NULL;
}

static void reserve_spot(int elf, int x, int y) {
  int d_x = x - prev_min_x + 1;
  int d_y = y - prev_min_y + 1;
  int offset = d_x * (prev_max_y - prev_min_y + 2) + d_y;
  dest_cache[offset] = elf;
}

static short get_reserved_spot(int x, int y) {
  int d_x = x - prev_min_x + 1;
  int d_y = y - prev_min_y + 1;
  int offset = d_x * (prev_max_y - prev_min_y + 2) + d_y;
  return dest_cache[offset];
}

static void get_dest(int x, int y, char dest, int *p_x, int *p_y) {
  switch(dest) {
    case NO_MOVE: *p_x = x;     *p_y = y; break;
    case NORTH:   *p_x = x;     *p_y = y - 1; break;
    case EAST:    *p_x = x + 1; *p_y = y; break;
    case SOUTH:   *p_x = x;     *p_y = y + 1; break;
    case WEST:    *p_x = x - 1; *p_y = y; break;
  }
}

static void plan_move(int num) {
  elf *e;
  int free_dirs[4];
  int i, prev_planned, planned_dir, p_x, p_y;
  
  if (!cache_elves) {
    e = malloc(sizeof(elf));
    fseek(elvesfp, num * sizeof(elf), SEEK_SET);
    fread(e, sizeof(elf), 1, elvesfp);
  } else {
    e = &elves[num];
  }

  prev_planned = e->planned_dir;

  free_dirs[NORTH] = !other_elves(e, NORTH);
  free_dirs[EAST]  = !other_elves(e, EAST);
  free_dirs[SOUTH] = !other_elves(e, SOUTH);
  free_dirs[WEST]  = !other_elves(e, WEST);

  e->planned_dir = NO_MOVE;

  if (free_dirs[NORTH] == 1 && free_dirs[EAST] == 1 &&
      free_dirs[SOUTH] == 1 && free_dirs[WEST] == 1) {
    goto save;
  }

  for (i = 0; i < 4; i++) {
    planned_dir = (prio_move + i) % 4;
    if (free_dirs[planned_dir]) {
      /* this one is free */
      goto update_plan;
    }
  }
  /* else there is no free direction */
  if (!cache_elves) free(e);
  return;
update_plan:
  e->planned_dir = planned_dir;
  
  get_dest(e->x, e->y, e->planned_dir, &p_x, &p_y);
  reserve_spot(num, p_x, p_y);
save:
  if (!cache_elves) {
    /* save elf */
    if (e->planned_dir != prev_planned) {
      fseek(elvesfp, num * sizeof(elf), SEEK_SET);
      fwrite(e, sizeof(elf), 1, elvesfp);
    }
    free(e);
  }
}

static int num_elf_moved = 0;

static void execute_move(int num) {
  int move_cancelled = 0;
  int p_x, p_y;
  elf *e;
  
  if (!cache_elves) {
    e = malloc(sizeof(elf));
    fseek(elvesfp, num * sizeof(elf), SEEK_SET);
    fread(e, sizeof(elf), 1, elvesfp);
  } else {
    e = &elves[num];
  }

  if (e->planned_dir == NO_MOVE) {
    if (!cache_elves) free(e);
    return;
  }

  get_dest(e->x, e->y, e->planned_dir, &p_x, &p_y);
  if (num != get_reserved_spot(p_x, p_y)) {
    /* got to cancel all p_x, p_y moves */
    move_cancelled = 1;
    e->planned_dir = NO_MOVE;
    /* let the next ones know there was a conflict */
    reserve_spot(num, p_x, p_y);
  }
  if (!move_cancelled) {
    num_elf_moved++;
    switch (e->planned_dir) {
      case NORTH: e->y--; break;
      case EAST:  e->x++; break;
      case SOUTH: e->y++; break;
      case WEST:  e->x--; break;
    }
    e->planned_dir = NO_MOVE;

    /* update bounds */
    if (e->x < min_x) {
      min_x = e->x;
    }
    if (e->y < min_y) {
      min_y = e->y;
    }
    if (e->x >= max_x) {
      max_x = e->x + 1;
    }
    if (e->y >= max_y) {
      max_y = e->y + 1;
    }
  }
  if (!cache_elves) {
    /* save elf */
    fseek(elvesfp, num * sizeof(elf), SEEK_SET);
    fwrite(e, sizeof(elf), 1, elvesfp);
    free(e);
  }
}

static void do_round(int round) {
  int i;
  cache_elves = max_x < 1000;

  if (!cache_elves) printf(" Building map...\n");

  build_cache(round);

  init_dest_cache();

  if (!cache_elves) printf(" Planning round...");
  for (i = 0; i < num_elves; i++) {
    plan_move(i);
  }

  bool_array_free(cache);
  cache = NULL;
  
  if (!cache_elves) printf("\n Executing round...");
  for (i = 0; i < num_elves; i++) {
    execute_move(i);
  }
  free_dest_cache();
  if (!cache_elves) printf("\n %d elves moved.\n", num_elf_moved);
  prio_move = (prio_move + 1) % 4;
}

static int free_tiles;
static void dump_map(int round) {
  int x, y, full;
  int show = max_x < 20;
  free_tiles = 0;

  for (y = min_y; y < max_y; y++) {
    if (show) gotoxy(20 - max_x/2 + min_x, y + 8 - max_y/2);
    for(x = min_x; x < max_x; x++) {
      full = has_elf(x, y);
      if (show) printf("%c", full ? '*':'.');
      if (!full) {
        free_tiles++;
      }
    }
    if (show) printf("\n");
  }

  if (round  == 10) {
    printf("\nWe have %d empty tiles after round 10.\n", free_tiles);
    printf("Hit a key to continue.");
#ifdef __CC65__
    cgetc();
    clrscr();
#endif
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
        elves[i].planned_dir = NO_MOVE;
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

  save_elves();

  do {
    num_elf_moved = 0;

    if (!cache_elves) {
      free(elves);
      elves = NULL;
    }
    
    do_round(round);
    printfat(0, 21, 1, "Finished round %d.\n", round + 1);
    round++;
  } while (num_elf_moved);

  printfat(0, 22, 1, "We did %d rounds.\n", round);

  fclose(elvesfp);
  unlink("ELVES");
  fclose(fp);
}
