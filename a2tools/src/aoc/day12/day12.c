#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef __CC65__
#include <conio.h>
#endif
#include "bfs.h"

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

static int get_neighbors_bfs (bfs *b, int node, int **neighbors);

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
  const int *path;
  int path_len;

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

  b = bfs_new(1);
  if (b == NULL) {
    exit(1);
  }
  printf("%d nodes (%d*%d)\n", max_x*max_y, max_x, max_y);
  bfs_set_get_neighbors_func(b, get_neighbors_bfs);

  if (bfs_set_grid(b, max_x, max_y) < 0) {
    exit(1);
  }

  path = bfs_grid_get_shortest_path(b, start_x, start_y, end_x, end_y, &path_len);
  for (i = 0; i < path_len; i++) {
    int cx, cy;
    bfs_node_to_grid(b, path[i], &cx, &cy);
    printf("(%d,%d) => ", cx, cy);
  }
  printf("\n");
  free(path);

  printf("\nPart1: %d,%d : %d\n", end_x, end_y, 
          bfs_grid_get_shortest_distance_to(b, start_x, start_y, end_x, end_y));

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
  printf("Part2: %d\n", closest_a);
  free_all();
#ifdef __CC65__
  cgetc();
#endif
  exit (0);
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
    *strchr(buf, '\n') = '\0';
    max_x = strlen(buf);

    nodes = realloc(nodes, (1 + max_y)*sizeof(char *));

    node_line = strdup(buf);

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

static void add_neighbor(bfs *b, int x, int y, int **neighbors, int *num_neighbors) {
  *neighbors = realloc(*neighbors, ((*num_neighbors) + 1) * sizeof(int));
  (*neighbors)[*num_neighbors] = bfs_grid_to_node(b, x, y);
  *num_neighbors = *num_neighbors + 1;
}

static int get_neighbors_bfs (bfs *b, int node, int **neighbors) {
  int x, y;
  char n;
  int num_neighbors = 0;

  bfs_node_to_grid(b, node, &x, &y);

  n = nodes[y][x];
  /* consider all directions */
  if (x > 0 && (nodes[y][x-1] >= n - 1)) {
    add_neighbor(b, x - 1, y, neighbors, &num_neighbors);
  }

  if (x < max_x - 1 && (nodes[y][x+1] >= n - 1)){
    add_neighbor(b, x + 1, y, neighbors, &num_neighbors);
  }

  if (y > 0 && (nodes[y-1][x] >= n - 1)){
    add_neighbor(b, x, y - 1, neighbors, &num_neighbors);
  }

  if (y < max_y - 1 && (nodes[y+1][x] >= n - 1)){
    add_neighbor(b, x, y + 1, neighbors, &num_neighbors);
  }

  return num_neighbors;
}
