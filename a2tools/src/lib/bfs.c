#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bfs.h"
#include "slist.h"

/* Warning : no copy of elements, caller has
 * to keep them allocated.
 */

struct _bfs {
  short **dests;

  int num_nodes;

  int *num_node_dests;
};

bfs *bfs_new(void) {
  bfs *b = malloc(sizeof(struct _bfs));
  b->dests = NULL;

  b->num_nodes = 0;
  b->num_node_dests = NULL;

  return b;
}

void bfs_free(bfs *b) {
  int i;
  for (i = 0; i < b->num_nodes; i++) {
    free(b->dests[i]);
  }
  free(b->dests);
  free(b->num_node_dests);
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

short *bfs_compute_shortest_paths(bfs *b, short start_node) {
  short *distances = malloc(b->num_nodes * sizeof(short));
  char *visited = malloc(b->num_nodes * sizeof(char));
  int i;
  short cur_len = 0;
  slist *queue = NULL;
  
  for (i = 0; i < b->num_nodes; i++) {
    visited[i] = 0;
    distances[i] = -1;
  }

  queue = slist_append(queue, (void *)(long)start_node);
  distances[start_node] = 0;
  visited[start_node] = 1;

  while (queue) {
    short next_node = (short)(long)(queue->data);

    queue = slist_remove(queue, queue);
    cur_len = distances[next_node] + 1;

    for (i = 0; i < b->num_node_dests[next_node]; i++) {
      short dest_node = b->dests[next_node][i];
      if (!visited[dest_node]) {
        distances[dest_node] = cur_len;
        visited[dest_node] = 1;
        queue = slist_append(queue, (void *)(long)dest_node);
      }
    }
    cur_len++;
  }
  free(visited);
  visited = NULL;

  return distances;
}
