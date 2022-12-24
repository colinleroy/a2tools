#ifndef __bfs_h
#define __bfs_h

typedef struct _bfs bfs;
struct _bfs {
  int **dests;

  int num_nodes;
  int *num_node_dests;

  /* for grid mode */
  int max_y;

  int start_node;
  int *distances;

  int trace_path;
  int *paths;
};

/* Must be freed by caller using bfs_free */
bfs *bfs_new(void);
void bfs_free(bfs *);

int bfs_add_nodes(bfs *b, int num_nodes);
int bfs_add_paths(bfs *b, int source, int *dest_nodes, int num_dests);

void bfs_enable_path_trace(bfs *b, int enable);

/* Not to be freed */
const int *bfs_compute_shortest_distances(bfs *b, int start_node);
int bfs_get_shortest_distance_to(bfs *b, int start_node, int end_node);
const int *bfs_get_shortest_path(bfs *b, int start_node, int end_node, int *path_len);

/* helper for grid. Use instead of bfs_add_nodes */

/* Used to flatten bidimensional array to unidimensional
 * for BFS */
#define SINGLE_DIM(x,y,y_len) (((x) * (y_len)) + (y))


int bfs_set_grid(bfs *b, int max_x, int max_y);

#define bfs_grid_to_node(b, x, y) \
  SINGLE_DIM(x, y, b->max_y)

void bfs_node_to_grid(bfs *b, int node, int *x, int *y);

#define bfs_grid_add_paths(b, x, y, dest_nodes, num_dests) \
  bfs_add_paths(b, SINGLE_DIM(x, y, b->max_y), dest_nodes, num_dests)

#define bfs_grid_compute_shortest_distances(b, x, y) \
  bfs_compute_shortest_distances(b, SINGLE_DIM(x, y, b->max_y))

#define bfs_grid_get_shortest_distance_to(b, start_x, start_y, end_x, end_y) \
  bfs_get_shortest_distance_to(b, SINGLE_DIM(start_x, start_y, b->max_y), SINGLE_DIM(end_x, end_y, b->max_y))

#define bfs_grid_get_shortest_path(b, start_x, start_y, end_x, end_y, path_len) \
  bfs_get_shortest_path(b, SINGLE_DIM(start_x, start_y, b->max_y), SINGLE_DIM(end_x, end_y, b->max_y), path_len)

#endif
