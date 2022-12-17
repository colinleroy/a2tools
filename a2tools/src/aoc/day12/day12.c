#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "day12.h"
#include "bfs.h"

// Djikstra implementation

#define DATASET "IN12"
#define BUFSIZE 255

/* Used to flatten bidimensional array to unidimensional
 * for BFS */
#define SINGLE_DIM(x,y,y_len) (((x) * (y_len)) + (y))

char buf[BUFSIZE];

char **nodes = NULL;

static void free_all(void);

int max_x = 0, max_y = 0;

int start_x, start_y;
int end_x, end_y;
int i, j;

static void read_file(FILE *fp);

static int build_neighbors_array(char n, int x, int y, short **neighbors_array);

static void find_closest_node(int *closest_x, int *closest_y);
static short *calculate_path_lengths();

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

int main(void) {
  FILE *fp;
  int closest_a = -1;
  short *bfs_dists = NULL;

#ifdef PRODOS_T_TXT
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

  bfs_dists = calculate_path_lengths();

  printf("\nPart1: Shortest path to %d,%d : %d\n", end_x, end_y, bfs_dists[SINGLE_DIM(end_x, end_y, max_y)]);

  for (i = 0; i < max_y; i++) {
    for (j = 0; j < max_x; j++) {
        if (nodes[i][j] == 'a' && bfs_dists[SINGLE_DIM(j, i, max_y)] >= 0) {
          if (closest_a < 0 || bfs_dists[SINGLE_DIM(j, i, max_y)] < closest_a) {
            closest_a = bfs_dists[SINGLE_DIM(j, i, max_y)];
          }
        }
    }
  }
  printf("Part2: Shortest path to an 'a' is %d\n", closest_a);
  free_all();
  free(bfs_dists);

  exit (0);
}

static short *calculate_path_lengths(void ) {
  short *bfs_dists = NULL;
  bfs *b = bfs_new();
  int x, y;
  printf("adding %d nodes(for map of %dx%d)\n", max_x*max_y, max_x, max_y);
  bfs_add_nodes(b, max_x * max_y);

  for (x = 0; x < max_x; x++) {
    for (y = 0; y < max_y; y++) {
      short *neighbors = NULL;
      int num_neighbors = build_neighbors_array(nodes[y][x], x, y, &neighbors);
      bfs_add_paths(b, SINGLE_DIM(x, y, max_y), neighbors, num_neighbors);
      free(neighbors);
    }
  }
  bfs_dists = bfs_compute_shortest_paths(b, SINGLE_DIM(start_x, start_y, max_y));
  bfs_free(b);

  return bfs_dists;
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

static int build_neighbors_array(char n, int x, int y, short **neighbors_array) {
  int num_neighbors = 0;
  short *neighbors = NULL;
  /* consider all directions */
  if (x > 0 && (nodes[y][x-1] >= n - 1)) {
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(short));
    neighbors[num_neighbors] = SINGLE_DIM(x-1, y, max_y);
    num_neighbors++;
  }

  if (x < max_x - 1 && (nodes[y][x+1] >= n - 1)){
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(short));
    neighbors[num_neighbors] = SINGLE_DIM(x+1, y, max_y);
    num_neighbors++;
  }

  if (y > 0 && (nodes[y-1][x] >= n - 1)){
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(short));
    neighbors[num_neighbors] = SINGLE_DIM(x, y-1, max_y);
    num_neighbors++;
  }

  if (y < max_y - 1 && (nodes[y+1][x] >= n - 1)){
    neighbors = realloc(neighbors, (num_neighbors + 1) * sizeof(short));
    neighbors[num_neighbors] = SINGLE_DIM(x, y+1, max_y);
    num_neighbors++;
  }
  *neighbors_array = neighbors;

  return num_neighbors;
}
