#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bfs.h"
#include "bool_array.h"

bfs *bfs_new(int enable_path_trace) {
  bfs *b = malloc(sizeof(bfs));
  if (b == NULL) {
    return NULL;
  }

  memset(b, 0, sizeof(bfs));
  b->trace_path = enable_path_trace;
  return b;
}

void bfs_free(bfs *b) {
  if (b->dests) {
    do {
      b->num_nodes--;
      free(b->dests[b->num_nodes]);
    } while (b->num_nodes > 0);
    free(b->dests);
    free(b->num_node_dests);
  }
  free(b->distances);
  free(b->paths);
  free(b);
}

int bfs_add_nodes(bfs *b, int num_nodes) {
  b->num_nodes = num_nodes;

  if (b->get_neighbors == NULL) {
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
  }
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

void bfs_set_get_neighbors_func(bfs *b, get_neighbors_func get_neighbors) {
  b->get_neighbors = get_neighbors;
}

static int *queue_in(int *queue, int *queue_len, int value) {
  queue = realloc(queue, (*queue_len + 1) * sizeof(int));
  queue[*queue_len] = value;
  *queue_len = *queue_len + 1;
  return queue;
}

static int *queue_out(int *queue, int *queue_len) {
  if (*queue_len == 1) {
    free(queue);
    *queue_len = 0;
    return NULL;
  }
  memmove(queue, queue + 1, (*queue_len - 1) * sizeof(int));
  queue = realloc(queue, (*queue_len) * sizeof(int));
  *queue_len = *queue_len - 1;
  
  return queue;
}

const int *bfs_compute_shortest_distances(bfs *b, int start_node) {
  char *visited;
  int i;
  int cur_len = 0;
  int *node_queue = NULL;
  int node_queue_len = 0;
  
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
  visited = malloc(b->num_nodes);
  if (visited == NULL) {
    return NULL;
  }
  memset(visited, 0, b->num_nodes);

  if (b->trace_path) {
    b->paths = malloc(b->num_nodes * sizeof(int));
    if (b->paths == NULL) {
      b->trace_path = 0;
    }
  }

  for (i = 0; i < b->num_nodes; i++) {
    b->distances[i] = -1;
  }

  node_queue = queue_in(node_queue, &node_queue_len, start_node);

  b->distances[start_node] = 0;
  visited[start_node] = 1;

  if (b->trace_path) {
    b->paths[start_node] = start_node;
  }

  while (node_queue_len) {
    int next_node = node_queue[0];
    int num_neighbors = 0;
    int *neighbors = NULL;
    node_queue = queue_out(node_queue, &node_queue_len);

    cur_len = b->distances[next_node] + 1;

    if (!b->get_neighbors) {
      num_neighbors = b->num_node_dests[next_node];
      neighbors = b->dests[next_node];
    } else {
      num_neighbors = b->get_neighbors(b, next_node, &neighbors);
    }
    for (i = 0; i < num_neighbors; i++) {
      int dest_node = neighbors[i];
      if (!visited[dest_node]) {
        b->distances[dest_node] = cur_len;
        visited[dest_node] = 1;

        if (b->trace_path) {
          b->paths[dest_node] = next_node;
        }

        node_queue = queue_in(node_queue, &node_queue_len, dest_node);
      }
    }
    if (b->get_neighbors) {
      free(neighbors);
    }
    cur_len++;
  }
  free(visited);

  return b->distances;
}

int bfs_get_shortest_distance_to(bfs *b, int start_node, int end_node) {
  bfs_compute_shortest_distances(b, start_node);
  return b->distances[end_node];
}

int *bfs_get_shortest_path(bfs *b, int start_node, int end_node, int *path_len) {
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
