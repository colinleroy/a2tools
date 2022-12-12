#include "slist.h"

typedef struct _node node;
typedef struct _path path;

struct _node {
  int x;
  int y;

  char height;

  slist *neighbors;
  
  int visited;
  path *shortest_path;
};

struct _path {
  int length;
  slist *steps;
};
