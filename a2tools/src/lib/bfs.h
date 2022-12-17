#ifndef __bfs_h
#define __bfs_h

typedef struct _bfs bfs;

/* Must be freed by caller using bfs_free */
bfs *bfs_new(void);
void bfs_free(bfs *);

void bfs_add_nodes(bfs *b, int num_nodes);
void bfs_add_paths(bfs *b, short source, short *dest_nodes, int num_dests);

/* Must be freed by caller using free */
short *bfs_compute_shortest_paths(bfs *b, short start_node);
#endif
