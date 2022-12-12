#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "day12.h"
#include "slist.h"

// Djikstra implementation

#define DATASET "IN12"

#define BUFSIZE 255
char buf[BUFSIZE];

node ***nodes = NULL;
int max_x = 0, max_y = 0;

node *start_node = NULL, *end_node = NULL;
node *closest_a = NULL;

static node ***read_file(FILE *fp);
static node *node_new(int x, int y, char height);
static void node_free(node *n);
static void nodes_free(node ***nodes);

static void build_all_neighbors(void);
#ifdef DEBUG
static void print_node_list(slist *list);
#endif
static path *path_new(void);
static path *add_step_to_path(path *p, node *n);
static void path_free(path *p);
static path *path_copy(path *p);

static node *find_closest_node(void);
static void calculate_path_lengths();

int main(void) {
  FILE *fp;
  int i,j;
  path *p;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(DATASET, "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }

  nodes = read_file(fp);

  fclose(fp);

  printf("read map of %d * %d\n", max_x, max_y);
  for (i = 0; i < max_y; i++) {
    for (j = 0; j < max_x; j++) {
      printf("%c",nodes[i][j]->height);
    }
    printf("\n");
  }

  build_all_neighbors();

#if DEBUG
  for (i = 0; i < max_x; i++) {
    for (j = 0; j < max_y; j++) {
      printf("neighbors of (%d,%d) of height %c:\n", i, j, nodes[j][i]->height);
      print_node_list(nodes[j][i]->neighbors);
    }
    printf("\n");
  }
#endif

  calculate_path_lengths();

#if DEBUG
  for (i = 0; i < max_y; i++) {
    for (j = 0; j < max_x; j++) {
      if (nodes[i][j]->visited == 0) {
        printf("We did not visit (%d,%d) :( \n", j, i);
      }
    }
  }
#endif

  printf("\nPart 1: Shortest path between %d,%d and %d,%d\n",
         start_node->x, start_node->y,
         end_node->x, end_node->y);

    for (i = 0; i < max_y; i++) {
      for (j = 0; j < max_x; j++) {
          if (nodes[i][j]->height == 'a' && nodes[i][j]->shortest_path != NULL) {
            if (closest_a == NULL 
              || nodes[i][j]->shortest_path->length < closest_a->shortest_path->length) {
              closest_a = nodes[i][j];
            }
          }
      }
    }

  printf("Part2: Shortest path to an 'a' is now to %d,%d : %d\n",
        closest_a->x,
        closest_a->y,
        closest_a->shortest_path->length);

  nodes_free(nodes);

  exit (0);
}

static node *find_closest_node(void) {
  int min_distance = -1;
  node *closest_node = NULL;
  int x, y;

  for (x = 0; x < max_x; x++) {
    for (y = 0; y < max_y; y++) {
      node *n = nodes[y][x];
      if (n->visited)
        continue;
      if (!n->shortest_path)
        continue;
      if (n->shortest_path->length <= min_distance || min_distance == -1) {
        min_distance = n->shortest_path->length;
        closest_node = n;
      }
    }
  }
  return closest_node;
}

static void calculate_path_lengths(void ) {
  int visited_count = 1;
  start_node->shortest_path = add_step_to_path(NULL, start_node);

  while (visited_count <= max_x*max_y) {
    node *cur = find_closest_node();
    slist *w;

    if (cur == NULL) {
      return;
    }

    cur->visited = 1;
    visited_count ++;

    printf("Visiting node (%d,%d)...\n", cur->x, cur->y);

    w = cur->neighbors;
    for(; w; w = w->next) {
      node *neighbor = (node *)w->data;

      if (!neighbor->visited && cur->shortest_path != NULL) {
        int cur_len = cur->shortest_path->length;
        int neighbor_len = -1;
        if (neighbor->shortest_path != NULL)
          neighbor_len = neighbor->shortest_path->length;
        if (cur_len + 1 < neighbor_len || neighbor_len < 0) {
          path_free(neighbor->shortest_path);

          neighbor->shortest_path = path_copy(cur->shortest_path);
          add_step_to_path(neighbor->shortest_path, neighbor);
#if DEBUG
          printf("%s shortest_path to (%d,%d) is now %d\n",
                 neighbor_len < 0 ? "New" : "Updated",
                 neighbor->x, neighbor->y, neighbor->shortest_path->length);
#endif
        }
      }
    }
  }
}

static node *node_new(int x, int y, char height) {
  node *n = malloc(sizeof(node));
  if (n == NULL) {
    printf("Couldn't allocate node (%d,%d)\n", x, y);
    exit(1);
  }
  n->x = x;
  n->y = y;
  n->height = height;
  n->neighbors = NULL;

  n->visited = 0;
  n->shortest_path = NULL;
  return n;
}

static void nodes_free(node ***n) {
  int i, j;
  for (i = 0; i < max_y; i++) {
    for (j = 0; j < max_x; j++) {
      node_free(n[i][j]);
    }
    free(n[i]);
  }
  free(n);
}

static void node_free(node *n) {
  if (n == NULL) {
    return;
  }
  slist_free(n->neighbors);
  path_free(n->shortest_path);
  free(n);
}

static path *path_new(void) {
  path *p = malloc(sizeof(path));
  if (p == NULL) {
    printf("Couldn't allocate path\n");
    exit(1);
  }
  p->steps = NULL;
  p->length = 0;
  return p;
}

static path *add_step_to_path(path *p, node *n) {
  if (p == NULL) {
    p = path_new();
  } else {
    p->length++;
  }
  p->steps = slist_append(p->steps, n);
  return p;
}

static void path_free(path *p) {
  if (p == NULL) {
    return;
  }
  slist_free(p->steps);
  free(p);
}

static path *path_copy(path *p) {
  path *new;
  if (p == NULL) {
    return NULL;
  }
  new = path_new();
  new->steps = slist_copy(p->steps);
  new->length = p->length;

  return new;
}

static node ***read_file(FILE *fp) {
  node ***nodes = NULL;
  node **node_line = NULL;
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

    nodes = realloc(nodes, (1 + max_y)*sizeof(node **));
    if (nodes == NULL) {
      printf("Couldn't realloc nodes\n");
      exit(1);
    }

    node_line = malloc( (1 + max_x)*sizeof(node *));
    if (node_line == NULL) {
      printf("Couldn't allocate node_line (%d)\n", 1 + max_x);
      exit(1);
    }
    nodes[max_y] = node_line;
    for (i = 0; i < max_x; i++) {
      node *n = node_new(i, max_y, buf[i]);

      if (buf[i] == 'S') {
        n->height = 'a';
        end_node = n;
      } else if (buf[i] == 'E') {
        n->height = 'z';
        start_node = n;
      }

      node_line[i] = n;
    }
    max_y++;
  }

  return nodes;
}

#ifdef DEBUG
static void print_node_list(slist *list) {
  slist *w = list;
  while (w != NULL) {
    node *n = (node *)w->data;
    printf(" (%d,%d) : %c", n->x, n->y, n->height);
    w = w->next;
  }
  printf("\n");
}
#endif

static slist *build_neighbors_list(node *n)
{ int x = n->x, y = n->y;
  slist *neighbors = NULL;

  /* consider all directions */
  if (x > 0 && (nodes[y][x-1]->height >= n->height - 1)) {
    neighbors = slist_prepend(neighbors, nodes[y][x-1]);
  }

  if (x < max_x - 1 && (nodes[y][x+1]->height >= n->height - 1)){
    neighbors = slist_prepend(neighbors, nodes[y][x+1]);
  }

  if (y > 0 && (nodes[y-1][x]->height >= n->height - 1)){
    neighbors = slist_prepend(neighbors, nodes[y-1][x]);
  }

  if (y < max_y - 1 && (nodes[y+1][x]->height >= n->height - 1)){
    neighbors = slist_prepend(neighbors, nodes[y+1][x]);
  }

  return neighbors;
}

static void build_all_neighbors(void) {
  int i, j;

  for (i = 0; i < max_y; i++) {
    for (j = 0; j < max_x; j++) {
      nodes[i][j]->neighbors = build_neighbors_list(nodes[i][j]);
    }
  }
}
