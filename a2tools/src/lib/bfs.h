#ifndef __bfs_h
#define __bfs_h

typedef struct _bfs bfs;

/* Must be freed by caller using bfs_free */
bfs *bfs_new(void);
void bfs_free(bfs *);

void bfs_add_nodes(bfs *b, int num_nodes);
void bfs_add_paths(bfs *b, short source, short *dest_nodes, int num_dests);

/* Not to be freed */
const short *bfs_compute_shortest_paths(bfs *b, short start_node);

/* helper for grid. Use instead of bfs_add_nodes */
void bfs_set_grid(bfs *b, int max_x, int max_y);
int bfs_grid_to_node(bfs *b, int x, int y);
void bfs_grid_add_paths(bfs *b, int x, int y, short *dest_nodes, int num_dests);

#endif
