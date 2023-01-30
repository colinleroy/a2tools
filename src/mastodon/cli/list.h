#ifndef __list_h
#define __list_h

#include <string.h>
#include "api.h"

typedef struct _list list;

#define L_HOME_TIMELINE   0
#define L_LOCAL_TIMELINE  1
#define L_PUBLIC_TIMELINE 2
#define L_FULL_STATUS     3
#define L_ACCOUNT         4
#define L_SEARCH          5

struct _list {
  char kind;
  char *root; /* For timelines */
  char *leaf_root;
  account *account; /* For accounts */
  char **ids;
  signed char first_displayed_post;
  status **displayed_posts;
  char last_displayed_post;
  signed char account_height;
  signed char *post_height;
  char n_posts;
  char eof;
  char scrolled;
};
#endif
