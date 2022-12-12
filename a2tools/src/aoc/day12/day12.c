#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "day12.h"
#include "slist.h"

// Djikstra implementation

#define DATASET "IN12"
#define BUFSIZE 255
#define DEBUG

char buf[BUFSIZE];

char **nodes = NULL;
char **visited = NULL;
int **distances = NULL;

static void free_all(void);

int max_x = 0, max_y = 0;

int start_x, start_y;
int end_x, end_y;
int i, j;

static void read_file(FILE *fp);

static slist *build_neighbors_list(char n, int x, int y);

static void find_closest_node(int *closest_x, int *closest_y);
static void calculate_path_lengths();

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
  printf("Visited map:\n");
  for (i = 0; i < max_y; i++) {
    for (j = 0; j < max_x; j++) {
      printf("%c",visited[i][j] == 0 ? '0':'1');
    }
    printf("\n");
  }
  printf("Distances map:\n");
  for (i = 0; i < max_y; i++) {
    for (j = 0; j < max_x; j++) {
      printf("%d ",distances[i][j]);
    }
    printf("\n");
  }
}
#endif

int main(void) {
  FILE *fp;
  int closest_a = -1;

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

  calculate_path_lengths();

  printf("\nPart1: Shortest path to %d,%d : %d\n", end_x, end_y, distances[end_y][end_x]);

  for (i = 0; i < max_y; i++) {
    for (j = 0; j < max_x; j++) {
        if (nodes[i][j] == 'a' && distances[i][j] >= 0) {
          if (closest_a < 0 || distances[i][j] < closest_a) {
            closest_a = distances[i][j];
            printf("Part2: Shortest path to an 'a' is now to %d,%d : %d\n", j, i, distances[i][j]);
          }
        }
    }
  }
  free_all();

  exit (0);
}

static void find_closest_node(int *closest_x, int *closest_y) {
  int min_distance = -1;
  int x, y;
  int c_v = 0, c_d = 0;
  *closest_x = -1;
  *closest_y = -1;

  for (x = 0; x < max_x; x++) {
    for (y = 0; y < max_y; y++) {
      if (visited[y][x]) {
        c_v++;
        continue;
      }
      if (distances[y][x] < 0) {
        c_d++;
        continue;
      }
      if (distances[y][x] <= min_distance || min_distance == -1) {
        min_distance = distances[y][x];
        *closest_x = x;
        *closest_y = y;
      }
    }
  }
}

static void calculate_path_lengths(void ) {
  int visited_count = 0;
  distances[start_y][start_x] = 0;

  printf("\n\nSTARTING.\n");

  while (1) {
    int cur_x = -1, cur_y = -1;
    slist *neighbors, *w;

    find_closest_node(&cur_x, &cur_y);

    if (cur_x < 0 && cur_y < 0) {
      return;
    }

    visited[cur_y][cur_x] = 1;
    visited_count ++;

    printf("Visiting node (%d,%d) (%d / %d visited)...\n", cur_x, cur_y, visited_count, max_x*max_y);

    neighbors = build_neighbors_list(nodes[cur_y][cur_x], cur_x, cur_y);
    for(w = neighbors; w; w = w->next) {
      int *neighbor = (int *)w->data;
      int neighbor_y = neighbor[0];
      int neighbor_x = neighbor[1];

      if (!visited[neighbor_y][neighbor_x] && distances[cur_y][cur_x] > -1) {
        int cur_len = distances[cur_y][cur_x];
        int neighbor_len = distances[neighbor_y][neighbor_x];

        if (cur_len + 1 < neighbor_len || neighbor_len < 0) {
          distances[neighbor_y][neighbor_x] = cur_len + 1;
#ifdef DEBUG
          printf("%s shortest_path to (%d,%d) is now %d\n",
                 neighbor_len < 0 ? "New" : "Updated",
                 neighbor_x, neighbor_y, distances[neighbor_y][neighbor_x]);
#endif
        }
      }
      free(neighbor);
    }
    slist_free(neighbors);
  }
}

static void free_all() {
  int i;
  for (i = 0; i < max_y; i++) {
    free(nodes[i]);
    free(visited[i]);
    free(distances[i]);
  }
  free(nodes);
  free(visited);
  free(distances);
}

static void read_file(FILE *fp) {
  char *node_line = NULL;
  char *visited_line = NULL;
  int *distances_line = NULL;
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

    nodes = realloc(nodes, (1 + max_y)*sizeof(char *));
    visited = realloc(visited, (1 + max_y)*sizeof(char *));
    distances = realloc(distances, (1 + max_y)*sizeof(int *));

    if (nodes == NULL) {
      printf("Couldn't realloc nodes\n");
      exit(1);
    }
    if (visited == NULL) {
      printf("Couldn't realloc visited\n");
      exit(1);
    }
    if (distances == NULL) {
      printf("Couldn't realloc distances\n");
      exit(1);
    }

    node_line = strdup(buf);
    visited_line = malloc( (1 + max_x)*sizeof(char));
    distances_line = malloc( (1 + max_x)*sizeof(int));
    if (node_line == NULL) {
      printf("Couldn't allocate node_line (%d)\n", 1 + max_x);
      exit(1);
    }
    if (visited_line == NULL) {
      printf("Couldn't allocate visited_line (%d)\n", 1 + max_x);
      exit(1);
    }
    if (distances_line == NULL) {
      printf("Couldn't allocate distances_line (%d)\n", 1 + max_x);
      exit(1);
    }

    nodes[max_y] = node_line;
    visited[max_y] = visited_line;
    distances[max_y] = distances_line;
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

      visited_line[i] = 0;
      distances_line[i] = -1;
    }
    max_y++;
  }
}

static slist *build_neighbors_list(char n, int x, int y) {
  slist *neighbors = NULL;

  /* consider all directions */
  if (x > 0 && (nodes[y][x-1] >= n - 1)) {
    int *coords = malloc(2*sizeof(int));
    coords[0] = y;
    coords[1] = x-1;
    neighbors = slist_prepend(neighbors, coords);
  }

  if (x < max_x - 1 && (nodes[y][x+1] >= n - 1)){
    int *coords = malloc(2*sizeof(int));
    coords[0] = y;
    coords[1] = x+1;
    neighbors = slist_prepend(neighbors, coords);
  }

  if (y > 0 && (nodes[y-1][x] >= n - 1)){
    int *coords = malloc(2*sizeof(int));
    coords[0] = y-1;
    coords[1] = x;
    neighbors = slist_prepend(neighbors, coords);
  }

  if (y < max_y - 1 && (nodes[y+1][x] >= n - 1)){
    int *coords = malloc(2*sizeof(int));
    coords[0] = y+1;
    coords[1] = x;
    neighbors = slist_prepend(neighbors, coords);
  }

  return neighbors;
}
