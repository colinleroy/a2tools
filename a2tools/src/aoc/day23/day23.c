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

static bool_array *cache = NULL;

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

static void build_cache(void) {
  elf *e = malloc(sizeof(elf));
  int i;
  cache = bool_array_alloc(max_x - min_x, max_y - min_y);

  fseek(elvesfp, 0, SEEK_SET);
  for (i = 0; i < num_elves; i++) {
    fread(e, sizeof(elf), 1, elvesfp);
    bool_array_set(cache, e->x - min_x, e->y - min_y, 1);
  }
  free(e);
}

static short **planned_positions;
static short *num_planned_positions_at;

static void init_plan_hash(void) {
  planned_positions = malloc((2 + max_x-min_x)*sizeof(short *));
  memset(planned_positions, 0, (2 + max_x-min_x)*sizeof(short *));
  num_planned_positions_at = malloc((2 + max_x - min_x)*sizeof(short));
  memset(num_planned_positions_at, 0, (2 + max_x - min_x)*sizeof(short));
}

static void add_plan_at_line(int elf, int x) {
  num_planned_positions_at[1 + x - min_x] ++;
  planned_positions[1 + x - min_x] = realloc(planned_positions[1 + x - min_x], num_planned_positions_at[1 + x - min_x] * sizeof(short));
  planned_positions[1 + x - min_x][num_planned_positions_at[1 + x - min_x] - 1] = elf;
}

static short *get_plans_at_line(int x, int *n) {
  *n = num_planned_positions_at[1 + x - min_x];
  return planned_positions[1 + x - min_x];
  
}

static void free_plan_hash(void) {
  int i = 0;
  for (i = 0; i < 2 + max_x - min_x; i++) {
    free(planned_positions[i]);
  }
  free(num_planned_positions_at);
  free(planned_positions);
}

static void plan_move(int num) {
  elf *e = malloc(sizeof(elf));
  int free_dirs[4];
  int i, planned_dir;
  
  if (num % 100 == 0) {
    printf(".");
  }
  fseek(elvesfp, num * sizeof(elf), SEEK_SET);
  fread(e, sizeof(elf), 1, elvesfp);

  free_dirs[NORTH] = !other_elves(e, NORTH);
  free_dirs[EAST]  = !other_elves(e, EAST);
  free_dirs[SOUTH] = !other_elves(e, SOUTH);
  free_dirs[WEST]  = !other_elves(e, WEST);

  if (free_dirs[NORTH] == 1 && free_dirs[EAST] == 1 &&
      free_dirs[SOUTH] == 1 && free_dirs[WEST] == 1) {
    free(e);
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
  free(e);
  return;
update_plan:
  switch(planned_dir) {
    case NORTH: e->p_y--; break;
    case SOUTH: e->p_y++; break;
    case EAST:  e->p_x++; break;
    case WEST:  e->p_x--; break;
  }

  /* save elf */
  fseek(elvesfp, num * sizeof(elf), SEEK_SET);
  fwrite(e, sizeof(elf), 1, elvesfp);

  /* save plan */
  add_plan_at_line(num, e->p_x);
  free(e);
}

static int num_elf_moved = 0;

static int new_min_x, new_min_y, new_max_x, new_max_y;

static void execute_move(int elf) {
  int i, move_cancelled = 0, n;
  short *other_plans_at_line = NULL;
  if (elf % 100 == 0) {
    printf(".");
  }

  if (elves[elf].p_x == elves[elf].x && elves[elf].p_y == elves[elf].y)
    return;

  other_plans_at_line = get_plans_at_line(elves[elf].p_x, &n);
  for (i = 0; i < n; i++) {
    int o_elf = other_plans_at_line[i];

    if (o_elf < 0 || o_elf == elf) {
      continue;
    } else if (elves[o_elf].p_x == elves[elf].p_x && elves[o_elf].p_y == elves[elf].p_y) {
      /* got to cancel all p_x, p_y moves */
      move_cancelled = 1;
      elves[o_elf].p_x = elves[o_elf].x;
      elves[o_elf].p_y = elves[o_elf].y;
      other_plans_at_line[i] = -1;
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
    if (elves[elf].x < new_min_x) {
      new_min_x = elves[elf].x;
    }
    if (elves[elf].y < new_min_y) {
      new_min_y = elves[elf].y;
    }
    if (elves[elf].x >= new_max_x) {
      new_max_x = elves[elf].x + 1;
    }
    if (elves[elf].y >= new_max_y) {
      new_max_y = elves[elf].y + 1;
    }
  }
}

static void do_round(void) {
  int i;

  printf(" Building map...\n");

  save_elves();
  free(elves);
  build_cache();

  init_plan_hash();

  printf(" Planning round...");
  for (i = 0; i < num_elves; i++) {
    plan_move(i);
  }

  printf("\n Freeing map and loading elves...");
  bool_array_free(cache);
  cache = NULL;
  read_elves();

  new_min_x = min_x;
  new_max_x = max_x;
  new_min_y = min_y;
  new_max_y = max_y;
  printf("\n Executing round...");
  for (i = 0; i < num_elves; i++) {
    execute_move(i);
  }
  free_plan_hash();

  min_x = new_min_x;
  max_x = new_max_x;
  min_y = new_min_y;
  max_y = new_max_y;

  printf("\n %d elves moved.\n", num_elf_moved);
  prio_move = (prio_move + 1) % 4;
}

static int free_tiles;
static void dump_map(void) {
  int x, y, full;
  free_tiles = 0;

  save_elves();
  free(elves);
  build_cache();

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
  bool_array_free(cache);
  cache = NULL;
  read_elves();
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
    num_elf_moved = 0;
    
    do_round();
    printf("Finished round %d.\n", round + 1);
    if (round + 1 == 10) {
      dump_map();
      printf("We have %d empty tiles.\n", free_tiles);
      printf("Hit a key to continue.");
      cgetc();
    }
    round++;
  } while (num_elf_moved);

  printf("We did %d rounds.\n", round);
  free(elves);

  fclose(fp);
}
