#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bfs.h"
#include "slist.h"

struct _bfs {
  short **dests;

  int num_nodes;
  int *num_node_dests;

  /* for grid mode */
  int max_y;

  int start_node;
  short *distances;
};

bfs *bfs_new(void) {
  bfs *b = malloc(sizeof(struct _bfs));
  b->dests = NULL;

  b->num_nodes = 0;
  b->num_node_dests = NULL;

  b->max_y = -1;
  b->start_node = -1;
  b->distances = NULL;
  return b;
}

void bfs_free(bfs *b) {
  int i;
  for (i = 0; i < b->num_nodes; i++) {
    free(b->dests[i]);
  }
  free(b->dests);
  free(b->num_node_dests);
  free(b->distances);
  free(b);
}

void bfs_add_nodes(bfs *b, int num_nodes) {
  b->num_nodes = num_nodes;

  b->dests = malloc(num_nodes * sizeof(short *));
  memset(b->dests, 0, num_nodes * sizeof(short));

  b->num_node_dests = malloc(num_nodes * sizeof(int));
  memset(b->num_node_dests, 0, num_nodes * sizeof(int));
}

void bfs_add_paths(bfs *b, short source, short *dest_nodes, int num_dests) {
  b->dests[source] = malloc(num_dests * sizeof(short));
  memcpy(b->dests[source], dest_nodes, num_dests * sizeof(short));
  b->num_node_dests[source] = num_dests;
}

const short *bfs_compute_shortest_paths(bfs *b, short start_node) {
  char *visited = malloc(b->num_nodes * sizeof(char));
  int i;
  short cur_len = 0;
  slist *queue = NULL;
  
  if (start_node == b->start_node && b->distances != NULL) {
    return b->distances;
  } else if (b->distances != NULL) {
    free(b->distances);
  }

  b->distances = malloc(b->num_nodes * sizeof(short));
  visited = malloc(b->num_nodes * sizeof(char));

  for (i = 0; i < b->num_nodes; i++) {
    visited[i] = 0;
    b->distances[i] = -1;
  }

  queue = slist_append(queue, (void *)(long)start_node);
  b->distances[start_node] = 0;
  visited[start_node] = 1;

  while (queue) {
    short next_node = (short)(long)(queue->data);

    queue = slist_remove(queue, queue);
    cur_len = b->distances[next_node] + 1;

    for (i = 0; i < b->num_node_dests[next_node]; i++) {
      short dest_node = b->dests[next_node][i];
      if (!visited[dest_node]) {
        b->distances[dest_node] = cur_len;
        visited[dest_node] = 1;
        queue = slist_append(queue, (void *)(long)dest_node);
      }
    }
    cur_len++;
  }
  free(visited);
  visited = NULL;

  return b->distances;
}

void bfs_set_grid(bfs *b, int max_x, int max_y) {
  b->max_y = max_y;
  bfs_add_nodes(b, max_x * max_y);
}

/* Used to flatten bidimensional array to unidimensional
 * for BFS */
#define SINGLE_DIM(x,y,y_len) (((x) * (y_len)) + (y))

int bfs_grid_to_node(bfs *b, int x, int y) {
  return SINGLE_DIM(x, y, b->max_y);
}

void bfs_grid_add_paths(bfs *b, int x, int y, short *dest_nodes, int num_dests) {
  bfs_add_paths(b, SINGLE_DIM(x, y, b->max_y), dest_nodes, num_dests);
}

const short *bfs_grid_compute_shortest_paths(bfs *b, int x, int y) {
  return bfs_compute_shortest_paths(b, SINGLE_DIM(x, y, b->max_y));
}

short bfs_grid_get_shortest_path_to(bfs *b, int start_x, int start_y, int end_x, int end_y) {
  bfs_compute_shortest_paths(b, SINGLE_DIM(start_x, start_y, b->max_y));
  return b->distances[SINGLE_DIM(end_x, end_y, b->max_y)];
}
