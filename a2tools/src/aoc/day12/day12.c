#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef __CC65__
#include <apple2.h>
#include <conio.h>
#endif
#include "day12.h"
#include "bfs.h"
#include "extended_conio.h"

// Djikstra implementation

#define DATASET "IN12"
#define BUFSIZE 255

char buf[BUFSIZE];

char **nodes = NULL;

static void free_all(void);

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
    for (j = 0; j < max_x; j++) {
      printf("%c",nodes[i][j]);
    }
    printf("\n");
  }
}
#endif

static bfs *b = NULL;

int main(void) {
  FILE *fp;
  int closest_a = -1;
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
    for (j = 0; j < max_x; j++) {
        if (nodes[i][j] == 'a') {
          int d = bfs_grid_get_shortest_distance_to(b, start_x, start_y, j, i); 
          if (d > -1 && (closest_a < 0 || d < closest_a)) {
            closest_a = d;
          }
        }
    }
  }
  bfs_free(b);
  printf("Part2: Shortest path to an 'a' is %d\n", closest_a);
  free_all();

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

  for (x = 0; x < max_x; x++) {
    for (y = 0; y < max_y; y++) {
      int *neighbors = NULL;
      int num_neighbors = build_neighbors_array(b, nodes[y][x], x, y, &neighbors);
      if (bfs_grid_add_paths(b, x, y, neighbors, num_neighbors) < 0) {
        printf("Cannot allocate BFS paths for (%d,%d) (%u remaining)\n", x, y, _heapmaxavail());
        cgetc();
        return;
      }
      free(neighbors);
    }
  }
}

static void free_all() {
  int i;
  for (i = 0; i < max_y; i++) {
    free(nodes[i]);
  }
  free(nodes);
}

static void read_file(FILE *fp) {
  char *node_line = NULL;
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
//HERE
    nodes = realloc(nodes, (1 + max_y)*sizeof(char *));

    if (nodes == NULL) {
      printf("Couldn't realloc nodes\n");
      exit(1);
    }

    node_line = strdup(buf);
    if (node_line == NULL) {
      printf("Couldn't allocate node_line (%d)\n", 1 + max_x);
      exit(1);
    }

    nodes[max_y] = node_line;
    for (i = 0; i < max_x; i++) {
      if (node_line[i] == 'S') {
        node_line[i] = 'a';
        end_x = i;
        end_y = max_y;
      } else if (node_line[i] == 'E') {
        node_line[i] = 'z';
        start_x = i;
        start_y = max_y;
      }
    }
    max_y++;
  }
}

static int build_neighbors_array(bfs *b, char n, int x, int y, int **neighbors_array) {
  int num_neighbors = 0;
  int *neighbors = NULL;
  /* consider all directions */
  if (x > 0 && (nodes[y][x-1] >= n - 1)) {
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(int));
    neighbors[num_neighbors] = bfs_grid_to_node(b, x-1, y);
    num_neighbors++;
  }

  if (x < max_x - 1 && (nodes[y][x+1] >= n - 1)){
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(int));
    neighbors[num_neighbors] = bfs_grid_to_node(b, x+1, y);
    num_neighbors++;
  }

  if (y > 0 && (nodes[y-1][x] >= n - 1)){
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(int));
    neighbors[num_neighbors] = bfs_grid_to_node(b, x, y-1);
    num_neighbors++;
  }

  if (y < max_y - 1 && (nodes[y+1][x] >= n - 1)){
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(int));
    neighbors[num_neighbors] = bfs_grid_to_node(b, x, y+1);
    num_neighbors++;
  }
  *neighbors_array = neighbors;

  return num_neighbors;
}
