#ifndef __list_h
#define __list_h

#include <string.h>
#include "api.h"

typedef struct _list list;

#define N_STATUS_TO_LOAD 10

struct _list {
  char kind;
  char *root; /* For timelines */
  char *leaf_root;
  account *account; /* For accounts */
  char *ids[N_STATUS_TO_LOAD];
  signed char first_displayed_post;
  item *displayed_posts[N_STATUS_TO_LOAD];
  char last_displayed_post;
  signed char account_height;
  signed char post_height[N_STATUS_TO_LOAD];
  char n_posts;
  char eof;
  char scrolled;
};
#endif
