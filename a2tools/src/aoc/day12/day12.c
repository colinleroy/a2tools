#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef __CC65__
#include <apple2.h>
#include <conio.h>
#include <em.h>
#endif
#include "day12.h"
#include "bfs.h"
#include "extended_conio.h"

// Djikstra implementation

#define DEBUG
#define DATASET "IN12E"
#define BUFSIZE 255

char buf[BUFSIZE];

int max_x = 0, max_y = 0;

int start_x, start_y;
int end_x, end_y;
int i, j;

static void read_file(FILE *fp);

static int build_neighbors_array(bfs *b, char n, int x, int y, int **neighbors_array);

static void setup_bfs();

#ifdef DEBUG
static void dump_maps(void) {
  int i, j;
  printf("nodes map of %d * %d\n", max_x, max_y);
  for (i = 0; i < max_y; i++) {
    char *em_nodeline = em_map(i);
    printf("loaded %d: %s\n", i, em_nodeline);
    for (j = 0; j < max_x; j++) {
      printf("%c", em_nodeline[j]);
    }
    printf("\n");
  }
}
#endif

static bfs *b = NULL;

int main(void) {
  FILE *fp;
  int closest_a = -1;
  int i, j;
  // int *path;
  // int path_len;

#ifdef PRODOS_T_TXT
  printf("allocable: %u\n", _heapmaxavail());
  cgetc();
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(DATASET, "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }

  if (em_install(a2_auxmem_emd) != 0) {
    printf("cannot install auxmem driver\n");
  }

  for (i = 0; i < 10; i++) {
    register unsigned *page = em_use(i);
    if (page == NULL) {
      printf("?????\n");
    }
    printf("writing %c in page %d\n", 'A'+i, i);
    for (j = 0; j < 10; j++) {
      page[j]=('A'+i+j);
    }
    page[j] = 0;
    em_commit();    
  }

  for (i = 0; i < 10; i++) {
    register unsigned *page = em_map(i);
    if (page == NULL) {
      printf("!!!!!!\n");
    }
    printf("read %s in page %d\n", page, i);
  }
  cgetc();
  read_file(fp);

  fclose(fp);

#ifdef DEBUG
  dump_maps();
#endif

  setup_bfs();

  printf("\nPart1: Shortest path to %d,%d : %d\n", end_x, end_y, 
          bfs_grid_get_shortest_distance_to(b, start_x, start_y, end_x, end_y));

  // path = bfs_grid_get_shortest_path(b, start_x, start_y, end_x, end_y, &path_len);
  // for (i = 0; i < path_len; i++) {
  //   int cx, cy;
  //   bfs_node_to_grid(b, path[i], &cx, &cy);
  //   printf("(%d,%d) => ", cx, cy);
  // }
  // printf("\n");
  // free(path);

  for (i = 0; i < max_y; i++) {
    char *em_nodeline = em_map(i);
    for (j = 0; j < max_x; j++) {
        if (em_nodeline[j] == 'a') {
          int d = bfs_grid_get_shortest_distance_to(b, start_x, start_y, j, i); 
          if (d > -1 && (closest_a < 0 || d < closest_a)) {
            closest_a = d;
          }
        }
    }
  }
  bfs_free(b);
  printf("Part2: Shortest path to an 'a' is %d\n", closest_a);

  exit (0);
}

static void setup_bfs(void ) {
  int x, y;

  b = bfs_new();
  if (b == NULL) {
    printf("Can't allocate bfs\n");
    exit(1);
  }
  printf("adding %d nodes(for map of %dx%d)\n", max_x*max_y, max_x, max_y);
  if (bfs_set_grid(b, max_x, max_y) < 0) {
    printf("Cannot allocate BFS grid\n");
    cgetc();
    return;
  }
  bfs_enable_path_trace(b, 1);

  for (y = 0; y < max_y; y++) {
    char *em_nodeline = em_map(y);
    for (x = 0; x < max_x; x++) {
      int *neighbors = NULL;
      int num_neighbors = build_neighbors_array(b, em_nodeline[x], x, y, &neighbors);
      if (bfs_grid_add_paths(b, x, y, neighbors, num_neighbors) < 0) {
        printf("Cannot allocate BFS paths for (%d,%d) (%u remaining)\n", x, y, _heapmaxavail());
        cgetc();
        return;
      }
      free(neighbors);
    }
  }
}

static void read_file(FILE *fp) {
  char *em_nodeline = NULL;
  int i;
  while (NULL != fgets(buf, sizeof(buf), fp)) {
    if (max_x == 0) {
      max_x = strlen(buf);
      /* is there a return line */
      if (buf[max_x - 1] == '\n') {
        max_x--;
      }
    }
    buf[max_x] = '\0';

    em_nodeline = em_map(max_y);
    memset(em_nodeline, 0, 256);
    strcpy(em_nodeline, buf);
    for (i = 0; i < max_x; i++) {
      if (em_nodeline[i] == 'S') {
        em_nodeline[i] = 'a';
        end_x = i;
        end_y = max_y;
      } else if (em_nodeline[i] == 'E') {
        em_nodeline[i] = 'z';
        start_x = i;
        start_y = max_y;
      }
    }
    printf("copied %s to %s at %d\n", buf, em_nodeline, max_y);
    em_commit();

    max_y++;
  }
}

static int build_neighbors_array(bfs *b, char n, int x, int y, int **neighbors_array) {
  int num_neighbors = 0;
  int *neighbors = NULL;
  char *em_nodeline = em_map(y);
  /* consider all directions */
  if (x > 0 && (em_nodeline[x-1] >= n - 1)) {
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(int));
    neighbors[num_neighbors] = bfs_grid_to_node(b, x-1, y);
    num_neighbors++;
  }

  if (x < max_x - 1 && (em_nodeline[x+1] >= n - 1)){
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(int));
    neighbors[num_neighbors] = bfs_grid_to_node(b, x+1, y);
    num_neighbors++;
  }

  if (y > 0) {
    em_nodeline = em_map(y - 1);
    if(em_nodeline[x] >= n - 1) {
      neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(int));
      neighbors[num_neighbors] = bfs_grid_to_node(b, x, y-1);
      num_neighbors++;
    }
  }

  if (y < max_y - 1) {
    em_nodeline = em_map(y + 1);
    if (em_nodeline[x] >= n - 1) {
      neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(int));
      neighbors[num_neighbors] = bfs_grid_to_node(b, x, y+1);
      num_neighbors++;
    }
  }
  *neighbors_array = neighbors;

  return num_neighbors;
}
