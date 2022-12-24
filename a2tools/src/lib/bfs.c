#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bfs.h"
#include "slist.h"
#include "bool_array.h"

bfs *bfs_new(void) {
  bfs *b = malloc(sizeof(bfs));
  if (b == NULL) {
    return NULL;
  }

  memset(b, 0, sizeof(bfs));

  return b;
}

void bfs_free(bfs *b) {
  do {
    b->num_nodes--;
    free(b->dests[b->num_nodes]);
  } while (b->num_nodes > 0);

  free(b->dests);
  free(b->num_node_dests);
  free(b->distances);
  free(b->paths);
  free(b);
}

int bfs_add_nodes(bfs *b, int num_nodes) {
  b->num_nodes = num_nodes;

  b->dests = malloc(num_nodes * sizeof(int *));
  if (b->dests == NULL) {
    return -1;
  }
  memset(b->dests, 0, num_nodes * sizeof(int));

  b->num_node_dests = malloc(num_nodes * sizeof(int));
  if (b->num_node_dests == NULL) {
    return -1;
  }
  memset(b->num_node_dests, 0, num_nodes * sizeof(int));
  return 0;
}

int bfs_add_paths(bfs *b, int source, int *dest_nodes, int num_dests) {
  b->num_node_dests[source] = num_dests;
  if (num_dests == 0) {
    return 0;
  }
  b->dests[source] = malloc(num_dests * sizeof(int));
  if (b->dests[source] == NULL) {
    return -1;
  }
  memcpy(b->dests[source], dest_nodes, num_dests * sizeof(int));

  return 0;
}

void bfs_enable_path_trace(bfs *b, int enable) {
  b->trace_path = enable;
}

const int *bfs_compute_shortest_distances(bfs *b, int start_node) {
  bool_array *visited;
  int i;
  int cur_len = 0;
  slist *queue = NULL;
  
  if (b->distances != NULL) {
    if (start_node == b->start_node) {
      return b->distances;
    } else {
      free(b->distances);
      free(b->paths);
      b->paths = NULL;
    }
  }

  b->start_node = start_node;
  b->distances = malloc(b->num_nodes * sizeof(int));
  if (b->distances == NULL) {
    return NULL;
  }
  visited = bool_array_alloc(b->num_nodes, 1); {
    if (visited == NULL) {
      return NULL;
    }
  }

  if (b->trace_path) {
    b->paths = malloc(b->num_nodes * sizeof(int));
    if (b->paths == NULL) {
      b->trace_path = 0;
    }
  }

  for (i = 0; i < b->num_nodes; i++) {
    bool_array_set(visited, i, 0, 0);
    b->distances[i] = -1;
  }

  queue = slist_append(queue, (void *)(long)start_node);
  b->distances[start_node] = 0;
  bool_array_set(visited, start_node, 0, 1);

  if (b->trace_path) {
    b->paths[start_node] = start_node;
  }

  while (queue) {
    int next_node = (int)(long)(queue->data);

    queue = slist_remove(queue, queue);
    cur_len = b->distances[next_node] + 1;

    for (i = 0; i < b->num_node_dests[next_node]; i++) {
      int dest_node = b->dests[next_node][i];
      if (!bool_array_get(visited, dest_node, 0)) {
        b->distances[dest_node] = cur_len;
        bool_array_set(visited, dest_node, 0, 1);

        if (b->trace_path) {
          b->paths[dest_node] = next_node;
        }

        queue = slist_append(queue, (void *)(long)dest_node);
      }
    }
    cur_len++;
  }
  bool_array_free(visited);

  return b->distances;
}

int bfs_get_shortest_distance_to(bfs *b, int start_node, int end_node) {
  bfs_compute_shortest_distances(b, start_node);
  return b->distances[end_node];
}

const int *bfs_get_shortest_path(bfs *b, int start_node, int end_node, int *path_len) {
  int i;
  int *path;
  int cur = end_node;
  *path_len = 1 + bfs_get_shortest_distance_to(b, start_node, end_node);

  path = malloc(*path_len * sizeof(int));
  if (path == NULL) {
    return NULL;
  }

  for (i = 0; i < *path_len; i++) {
    path[(*path_len - i - 1)] = cur;
    cur = b->paths[cur];
  }

  return path;
}

int bfs_set_grid(bfs *b, int max_x, int max_y) {
  b->max_y = (max_y);
  return bfs_add_nodes(b, (max_x) * (max_y));
}

void bfs_node_to_grid(bfs *b, int node, int *x, int *y) {
  *x = node / b->max_y;
  *y = node - (*x * b->max_y);
}
