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

  int trace_path;
  short *paths;
};

bfs *bfs_new(void) {
  bfs *b = malloc(sizeof(struct _bfs));
  b->dests = NULL;

  b->num_nodes = 0;
  b->num_node_dests = NULL;

  b->max_y = -1;
  b->start_node = -1;
  b->distances = NULL;
  
  b->trace_path = 0;
  b->paths = NULL;
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
  free(b->paths);
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

void bfs_enable_path_trace(bfs *b, int enable) {
  b->trace_path = enable;
}

const short *bfs_compute_shortest_distances(bfs *b, short start_node) {
  char *visited;
  int i;
  short cur_len = 0;
  slist *queue = NULL;
  
  if (start_node == b->start_node && b->distances != NULL) {
    return b->distances;
  } else if (b->distances != NULL) {
    free(b->distances);
    free(b->paths);
    b->paths = NULL;
  }

  b->start_node = start_node;
  b->distances = malloc(b->num_nodes * sizeof(short));
  visited = malloc(b->num_nodes * sizeof(char));

  if (b->trace_path) {
    b->paths = malloc(b->num_nodes * sizeof(short));
  }

  for (i = 0; i < b->num_nodes; i++) {
    visited[i] = 0;
    b->distances[i] = -1;
  }

  queue = slist_append(queue, (void *)(long)start_node);
  b->distances[start_node] = 0;
  visited[start_node] = 1;

  if (b->trace_path) {
    b->paths[start_node] = start_node;
  }

  while (queue) {
    short next_node = (short)(long)(queue->data);

    queue = slist_remove(queue, queue);
    cur_len = b->distances[next_node] + 1;

    for (i = 0; i < b->num_node_dests[next_node]; i++) {
      short dest_node = b->dests[next_node][i];
      if (!visited[dest_node]) {
        b->distances[dest_node] = cur_len;
        visited[dest_node] = 1;

        if (b->trace_path) {
          b->paths[dest_node] = next_node;
        }

        queue = slist_append(queue, (void *)(long)dest_node);
      }
    }
    cur_len++;
  }
  free(visited);
  visited = NULL;

  return b->distances;
}

short bfs_get_shortest_distance_to(bfs *b, short start_node, short end_node) {
  bfs_compute_shortest_distances(b, start_node);
  return b->distances[end_node];
}

const short *bfs_get_shortest_path(bfs *b, short start_node, short end_node, int *path_len) {
  int i;
  short *path;
  short cur = end_node;
  *path_len = 1 + bfs_get_shortest_distance_to(b, start_node, end_node);

  path = malloc(*path_len * sizeof(short));

  for (i = 0; i < *path_len; i++) {
    path[(*path_len - i - 1)] = cur;
    cur = b->paths[cur];
  }

  return path;
}

/* Used to flatten bidimensional array to unidimensional
 * for BFS */
#define SINGLE_DIM(x,y,y_len) (((x) * (y_len)) + (y))

const short *bfs_grid_get_shortest_path(bfs *b, int start_x, int start_y, int end_x, int end_y, int *path_len) {
  return bfs_get_shortest_path(b, SINGLE_DIM(start_x, start_y, b->max_y), SINGLE_DIM(end_x, end_y, b->max_y), path_len);
}

void bfs_set_grid(bfs *b, int max_x, int max_y) {
  b->max_y = max_y;
  bfs_add_nodes(b, max_x * max_y);
}

int bfs_grid_to_node(bfs *b, int x, int y) {
  return SINGLE_DIM(x, y, b->max_y);
}

void bfs_node_to_grid(bfs *b, int node, int *x, int *y) {
  *x = node / b->max_y;
  *y = node - (*x * b->max_y);
}

void bfs_grid_add_paths(bfs *b, int x, int y, short *dest_nodes, int num_dests) {
  bfs_add_paths(b, SINGLE_DIM(x, y, b->max_y), dest_nodes, num_dests);
}

const short *bfs_grid_compute_shortest_distances(bfs *b, int x, int y) {
  return bfs_compute_shortest_distances(b, SINGLE_DIM(x, y, b->max_y));
}

short bfs_grid_get_shortest_distance_to(bfs *b, int start_x, int start_y, int end_x, int end_y) {
  return bfs_get_shortest_distance_to(b, SINGLE_DIM(start_x, start_y, b->max_y), SINGLE_DIM(end_x, end_y, b->max_y));
}
